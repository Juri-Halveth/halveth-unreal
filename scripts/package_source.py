"""Export the explicit public-source surface; no engine, build output or runtime state."""
from __future__ import annotations

import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import stat
import struct
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
ROOT_FILES = frozenset({
    "HALVETHRealms.uproject", "README.md", "LICENSE", "THIRD-PARTY-NOTICES.md", ".gitignore", ".gitattributes",
    "GARDEN.sh", "CHARACTERS.sh", "requirements-art.txt",
})
TREE_EXTENSIONS = {
    "Source": {".h", ".cpp", ".cs", ".lua", ".omwscripts"},
    "Config": {".ini"},
    "scripts": {".py", ".ps1"},
    "tests": {".py", ".cpp", ".json"},
    "docs": {".md", ".txt", ".json", ".png", ".jpg", ".jpeg", ".svg"},
    "assets": {".svg", ".png", ".ico", ".md", ".txt", ".json"},
    "ArtSource": {".png", ".md", ".txt", ".json", ".fbx", ".obj", ".mhskel", ".mhw", ".mhclo", ".mhmat", ".target"},
    "Preview": {".png"},
    "QA": {".json"},
    "installer": {".nsi"},
}
EXACT_EXTRA_FILES = frozenset({"Build/Windows/Application.ico", ".github/workflows/source.yml"})
TEXT_EXTENSIONS = {".h", ".cpp", ".cs", ".ini", ".py", ".ps1", ".json", ".md", ".txt", ".svg", ".yml", ".nsi", ".sh", ".obj", ".mhskel", ".mhw", ".mhclo", ".mhmat", ".target", ".lua", ".omwscripts"}
BLOCKED_PARTS = frozenset({"__pycache__", ".git", ".vs", ".idea", "node_modules"})
SECRET_RULES = (
    ("credential_assignment", re.compile(
        r'(?im)^\s*["\']?(?:SecurityToken|ApiKey|Api[_-]Key|ClientSecret|Client[_-]Secret|'
        r'AccessToken|RefreshToken|PrivateKey|Password|Secret)["\']?\s*[:=]\s*\S+')),
    ("absolute_user_directory", re.compile(r"(?i)(?:[a-z]:[\\/]+Users[\\/]+|/" + "Users/|/" + "home/)")),
    ("private_key_block", re.compile(r"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----")),
    ("credential_token", re.compile(r"\b(?:gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|sk-(?:proj-)?[A-Za-z0-9_-]{24,})\b")),
)
PNG_SIGNATURE = bytes([137, 80, 78, 71, 13, 10, 26, 10])


class ExportError(ValueError):
    pass


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def reject_link(path: Path) -> None:
    if path.is_symlink() or (hasattr(path, "is_junction") and path.is_junction()):
        raise ExportError("Linked source entries are not exported: " + path.name)
    info = path.lstat()
    if getattr(info, "st_file_attributes", 0) & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0):
        raise ExportError("Reparse source entries are not exported: " + path.name)


def allowed(relative: PurePosixPath) -> bool:
    name = relative.as_posix()
    if name in ROOT_FILES or name in EXACT_EXTRA_FILES:
        return True
    if len(relative.parts) < 2 or relative.parts[0] not in TREE_EXTENSIONS:
        return False
    if any(part in BLOCKED_PARTS or part.startswith(".") or part.lower().endswith(".fbm") for part in relative.parts[1:]):
        return False
    # Native frame sequences are local capture evidence. Publication selects
    # reviewed root-level previews, rather than exporting every recorded frame.
    if relative.parts[0] == "Preview" and len(relative.parts) != 2:
        return False
    return relative.suffix.lower() in TREE_EXTENSIONS[relative.parts[0]]


def candidates(root: Path) -> list[Path]:
    root = root.resolve(strict=True)
    paths: list[Path] = []
    for name in sorted(ROOT_FILES | EXACT_EXTRA_FILES):
        path = root / name
        if not path.is_file():
            raise ExportError("Required public source is missing: " + name)
        for parent in [path, *path.parents]:
            if parent == root:
                break
            reject_link(parent)
        paths.append(path)
    for folder in TREE_EXTENSIONS:
        start = root / folder
        if not start.is_dir():
            raise ExportError("Required public source folder is missing: " + folder)
        reject_link(start)
        for current, directories, filenames in os.walk(start, followlinks=False):
            current_path = Path(current)
            for directory in list(directories):
                child = current_path / directory
                reject_link(child)
                if directory in BLOCKED_PARTS or directory.startswith("."):
                    directories.remove(directory)
            for filename in filenames:
                path = current_path / filename
                relative = PurePosixPath(path.relative_to(root).as_posix())
                if allowed(relative):
                    reject_link(path)
                    if not path.resolve(strict=True).is_relative_to(root):
                        raise ExportError("Source entry resolves outside the project: " + relative.as_posix())
                    paths.append(path)
    return sorted(set(paths), key=lambda path: path.relative_to(root).as_posix())


def validate_text(relative: str, data: bytes) -> None:
    if PurePosixPath(relative).suffix.lower() not in TEXT_EXTENSIONS and relative not in ROOT_FILES:
        return
    try:
        text = data.decode("utf-8-sig")
    except UnicodeDecodeError as exc:
        raise ExportError("Public text must be UTF-8: " + relative) from exc
    for name, pattern in SECRET_RULES:
        if pattern.search(text):
            # Report only rule and path; never echo a matching value.
            raise ExportError("Public text rule " + name + " rejected " + relative)


def validate_art(files: dict[str, bytes]) -> dict:
    manifest_name = "ArtSource/PolyHaven/MANIFEST.json"
    try:
        manifest = json.loads(files[manifest_name].decode("utf-8-sig"))
    except (KeyError, ValueError) as exc:
        raise ExportError("Poly Haven source manifest is missing or invalid") from exc
    if manifest.get("license") != "CC0-1.0":
        raise ExportError("This exporter expects the recorded CC0 Poly Haven source set")
    listed: set[str] = set()
    byte_count = 0
    for item in manifest.get("files", []):
        raw = item.get("relativePath", "")
        relative = PurePosixPath(raw)
        if not raw or "\\" in raw or relative.is_absolute() or ".." in relative.parts or ":" in raw:
            raise ExportError("Invalid source-art manifest path")
        name = "ArtSource/PolyHaven/" + relative.as_posix()
        if name in listed:
            raise ExportError("Duplicate source-art manifest path: " + name)
        listed.add(name)
        data = files.get(name)
        if data is None:
            raise ExportError("Manifested source art is missing: " + name)
        if len(data) != item.get("bytes") or len(data) != item.get("expectedBytes") or sha256(data) != item.get("sha256"):
            raise ExportError("Source-art byte/hash mismatch: " + name)
        if len(data) < 33 or data[:8] != PNG_SIGNATURE or data[12:16] != b"IHDR":
            raise ExportError("Source-art PNG header mismatch: " + name)
        width, height = struct.unpack(">II", data[16:24])
        if (width, height) != (item.get("width"), item.get("height")):
            raise ExportError("Source-art PNG dimensions mismatch: " + name)
        byte_count += len(data)
    character_prefix = "ArtSource/Characters/"
    character_manifest = character_prefix + "MANIFEST.json"
    character_files = {name for name in files if name.startswith(character_prefix)}
    if character_files:
        try:
            inventory = json.loads(files[character_manifest].decode("utf-8-sig"))
        except (KeyError, ValueError) as exc:
            raise ExportError("Character source manifest is missing or invalid") from exc
        if inventory.get("license") != "CC0-1.0":
            raise ExportError("Character source inventory must bind its CC0 asset license")
        declared = set()
        for item in inventory.get("files", []):
            raw = item.get("path", "")
            relative = PurePosixPath(raw)
            if not raw or "\\" in raw or relative.is_absolute() or ".." in relative.parts or ":" in raw:
                raise ExportError("Invalid character source manifest path")
            name = character_prefix + raw
            if name in declared:
                raise ExportError("Duplicate character source path")
            declared.add(name)
            data = files.get(name)
            if data is None or len(data) != item.get("bytes") or sha256(data) != item.get("sha256"):
                raise ExportError("Character source byte/hash mismatch: " + name)
        if declared | {character_manifest} != character_files:
            raise ExportError("Every character source asset must belong to its bound inventory")
    actual = {name for name in files if name.startswith("ArtSource/") and not name.startswith(character_prefix) and name.lower().endswith(".png")}
    if listed != actual or not listed:
        raise ExportError("Every source-art PNG must belong to the reviewed Poly Haven manifest")
    if len(listed) != manifest.get("filesCount") or byte_count != manifest.get("downloadedMapBytes"):
        raise ExportError("Source-art manifest totals do not match the files")
    return {"license": "CC0-1.0", "files": len(listed), "bytes": byte_count,
            "manifestSha256": sha256(files[manifest_name]), "characterSourceFiles": len(character_files)}


def snapshot(root: Path = ROOT, maximum_bytes: int = 512 * 1024 * 1024) -> tuple[dict[str, bytes], dict]:
    root = root.resolve(strict=True)
    files: dict[str, bytes] = {}
    total = 0
    for path in candidates(root):
        relative = path.relative_to(root).as_posix()
        size = path.stat().st_size
        if total + size > maximum_bytes:
            raise ExportError("Public source exceeds the configured byte budget")
        data = path.read_bytes()
        total += len(data)
        if total > maximum_bytes:
            raise ExportError("Public source changed beyond the configured byte budget")
        validate_text(relative, data)
        files[relative] = data
    art = validate_art(files)
    manifest = {
        "schema": "halveth.public-source-export.v1",
        "engineIncluded": False,
        "generatedUnrealAssetsIncluded": False,
        "privateRuntimeStateIncluded": False,
        "sourceBytes": total,
        "sourceFiles": len(files),
        "art": art,
        "files": [{"path": name, "bytes": len(data), "sha256": sha256(data)} for name, data in files.items()],
    }
    return files, manifest


def manifest_bytes(manifest: dict) -> bytes:
    return (json.dumps(manifest, indent=2, ensure_ascii=False, sort_keys=True) + "\n").encode("utf-8")


def write_archive(root: Path, files: dict[str, bytes], manifest: dict, version: str) -> tuple[Path, str]:
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+(?:-[a-zA-Z0-9.-]+)?", version):
        raise ExportError("Version must be a three-part release version")
    export_manifest = manifest_bytes(manifest)
    identity = sha256(export_manifest)[:12]
    destination = root.resolve(strict=True) / "dist"
    if destination.exists():
        reject_link(destination)
    destination.mkdir(exist_ok=True)
    target = destination / f"HALVETH-Portal-Garden-{version}-source-{identity}.zip"
    with tempfile.NamedTemporaryFile(prefix="source-", suffix=".part", dir=destination, delete=False) as temporary:
        temporary_path = Path(temporary.name)
    try:
        with zipfile.ZipFile(temporary_path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
            for name, data in [*files.items(), ("SOURCE-MANIFEST.json", export_manifest)]:
                info = zipfile.ZipInfo("HALVETH-Unreal/" + name, date_time=(1980, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.create_system = 3
                info.external_attr = 0o100644 << 16
                archive.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=6)
        digest = sha256(temporary_path.read_bytes())
        if target.exists():
            reject_link(target)
            if sha256(target.read_bytes()) != digest:
                raise ExportError("An existing content-addressed source archive has different bytes")
            temporary_path.unlink()
        else:
            temporary_path.replace(target)
        checksum = target.with_suffix(".zip.sha256")
        if checksum.exists():
            reject_link(checksum)
        checksum.write_text(f"{digest}  {target.name}\n", encoding="utf-8")
        return target, digest
    finally:
        if temporary_path.exists():
            temporary_path.unlink()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Validate and summarize without writing an archive")
    parser.add_argument("--version", default=None, help="Release version; defaults to Config/DefaultGame.ini")
    parser.add_argument("--max-source-mib", type=int, default=512, help="Explicit source byte limit (default 512 MiB)")
    args = parser.parse_args()
    try:
        if args.max_source_mib <= 0:
            raise ExportError("Source byte budget must be positive")
        files, manifest = snapshot(ROOT, args.max_source_mib * 1024 * 1024)
        result = {"sourceExportPassed": True, "sourceFiles": manifest["sourceFiles"],
                  "sourceBytes": manifest["sourceBytes"], "art": manifest["art"]}
        if not args.check:
            settings = configparser.ConfigParser(strict=False, interpolation=None)
            settings.read(ROOT / "Config/DefaultGame.ini", encoding="utf-8-sig")
            version = args.version or settings["/Script/EngineSettings.GeneralProjectSettings"]["ProjectVersion"]
            target, digest = write_archive(ROOT, files, manifest, version)
            result.update({"archive": target.relative_to(ROOT).as_posix(), "sha256": digest,
                           "archiveBytes": target.stat().st_size})
        print(json.dumps(result, indent=2))
        return 0
    except (ExportError, OSError, ValueError, KeyError) as exc:
        print(json.dumps({"sourceExportPassed": False, "error": str(exc)}, indent=2), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
