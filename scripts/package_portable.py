"""Build a deterministic portable runtime ZIP from an explicit installer manifest.

Python 3.13+; standard library only. The manifest and package root are required.
Only payloadFiles are read. This creates no installer, uninstall registration,
or player-save snapshot. An existing archive or checksum is never replaced.
Reproducibility means identical inputs under the same Python/zlib versions.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import sys
import tempfile
import zipfile
import zlib


ROOT = Path(__file__).resolve().parents[1]
INSTALL_SCHEMA = "halveth.portal-garden.install-manifest/1"
PORTABLE_SCHEMA = "halveth.portal-garden.portable-manifest/1"
PRODUCT = "HALVETH Portal Garden"
CHUNK_BYTES = 1024 * 1024
MAX_MANIFEST_BYTES = 16 * 1024 * 1024
FIXED_TIME = (1980, 1, 1, 0, 0, 0)
RESERVED = re.compile(r"^(?:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)", re.I)


class PackageError(ValueError):
    pass


def reject_link(path: Path) -> os.stat_result:
    info = path.lstat()
    if stat.S_ISLNK(info.st_mode) or (
        getattr(info, "st_file_attributes", 0)
        & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0)
    ):
        raise PackageError("Filesystem link/reparse point rejected: " + path.name)
    if stat.S_ISREG(info.st_mode) and info.st_nlink != 1:
        raise PackageError("Hard-linked file rejected: " + path.name)
    return info


def plain_absolute(path: Path, *, directory: bool) -> Path:
    absolute = Path(os.path.abspath(path))
    for entry in [*reversed(absolute.parents), absolute]:
        reject_link(entry)
    mode = absolute.lstat().st_mode
    if directory and not stat.S_ISDIR(mode):
        raise PackageError("Expected an existing plain directory: " + absolute.name)
    if not directory and not stat.S_ISREG(mode):
        raise PackageError("Expected an existing regular file: " + absolute.name)
    return absolute.resolve(strict=True)


def normalized_payload_path(raw: object) -> str:
    if not isinstance(raw, str) or not raw or len(raw) > 1024:
        raise PackageError("Payload paths must be nonempty relative strings")
    if "\\" in raw and "/" in raw:
        raise PackageError("Mixed path separators rejected")
    separator = "\\" if "\\" in raw else "/"
    parts = raw.split(separator)
    for part in parts:
        if (
            part in ("", ".", "..")
            or part[-1:] in (" ", ".")
            or any(ord(char) < 32 or ord(char) == 127 for char in part)
            or any(char in '<>:"|?*' for char in part)
            or RESERVED.match(part)
        ):
            raise PackageError("Non-normal relative payload path rejected")
        try:
            part.encode("utf-8", errors="strict")
        except UnicodeEncodeError as exc:
            raise PackageError("Payload paths must contain valid Unicode") from exc
    result = "/".join(parts)
    if result.casefold() == "install-manifest.json" or parts[-1].casefold().startswith("uninstall"):
        raise PackageError("Installer-generated files are not portable payload: " + result)
    return result


def no_duplicate_keys(pairs: list[tuple[str, object]]) -> dict:
    result: dict = {}
    for key, value in pairs:
        if key in result:
            raise PackageError("Duplicate JSON object key rejected: " + key)
        result[key] = value
    return result


def load_manifest(path: Path) -> tuple[dict, list[dict], str]:
    path = plain_absolute(path, directory=False)
    if path.stat().st_size > MAX_MANIFEST_BYTES:
        raise PackageError("Installer manifest exceeds the 16 MiB limit")
    with path.open("rb") as source:
        data = source.read(MAX_MANIFEST_BYTES + 1)
    if len(data) > MAX_MANIFEST_BYTES:
        raise PackageError("Installer manifest grew beyond the 16 MiB limit")
    manifest = json.loads(data.decode("utf-8-sig"), object_pairs_hook=no_duplicate_keys)
    if not isinstance(manifest, dict) or manifest.get("schema") != INSTALL_SCHEMA:
        raise PackageError("Expected an installer-generated install-manifest/1")
    if manifest.get("product") != PRODUCT:
        raise PackageError("Installer manifest product does not match this project")
    version = manifest.get("version")
    if not isinstance(version, str) or not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version):
        raise PackageError("Manifest version must be a three-part numeric version")
    entries = manifest.get("payloadFiles")
    if not isinstance(entries, list) or not entries:
        raise PackageError("Installer manifest must contain a nonempty payloadFiles array")
    files = []
    seen: set[str] = set()
    for item in entries:
        if not isinstance(item, dict) or set(item) != {"path", "bytes", "sha256"}:
            raise PackageError("Each payload entry must contain exactly path, bytes and sha256")
        name = normalized_payload_path(item["path"])
        if name.casefold() in seen:
            raise PackageError("Case-insensitive duplicate payload path: " + name)
        seen.add(name.casefold())
        size, digest = item["bytes"], item["sha256"]
        if type(size) is not int or size < 0:
            raise PackageError("Payload byte counts must be nonnegative integers: " + name)
        if not isinstance(digest, str) or not re.fullmatch(r"[0-9a-fA-F]{64}", digest):
            raise PackageError("Payload SHA-256 must be 64 hexadecimal characters: " + name)
        files.append({"path": name, "bytes": size, "sha256": digest.lower()})
    for name in seen:
        parts = name.split("/")
        if any("/".join(parts[:index]) in seen for index in range(1, len(parts))):
            raise PackageError("A payload file collides with a directory path: " + name)
    return manifest, sorted(files, key=lambda item: item["path"]), hashlib.sha256(data).hexdigest()


def payload_source(root: Path, name: str) -> tuple[Path, os.stat_result]:
    current = root
    reject_link(current)
    parts = name.split("/")
    for index, part in enumerate(parts):
        current = current / part
        info = reject_link(current)
        expected_type = stat.S_ISREG if index == len(parts) - 1 else stat.S_ISDIR
        if not expected_type(info.st_mode):
            raise PackageError("Payload is not a regular file with plain parent directories: " + name)
    if not current.resolve(strict=True).is_relative_to(root):
        raise PackageError("Payload resolves outside the explicit package root: " + name)
    return current, info


def zip_info(name: str, size: int) -> zipfile.ZipInfo:
    info = zipfile.ZipInfo(name, date_time=FIXED_TIME)
    info.compress_type = zipfile.ZIP_DEFLATED
    info.compress_level = 6
    info.create_system = 3
    info.external_attr = 0o100644 << 16
    info.file_size = size
    return info


def stream_payload(archive: zipfile.ZipFile, root: Path, prefix: str, item: dict) -> None:
    path, before = payload_source(root, item["path"])
    if before.st_size != item["bytes"]:
        raise PackageError("Payload size mismatch before reading: " + item["path"])
    flags = os.O_RDONLY | getattr(os, "O_BINARY", 0) | getattr(os, "O_NOFOLLOW", 0)
    descriptor = os.open(path, flags)
    with os.fdopen(descriptor, "rb") as source:
        opened = os.fstat(source.fileno())
        if (
            not stat.S_ISREG(opened.st_mode)
            or opened.st_nlink != 1
            or (opened.st_dev, opened.st_ino) != (before.st_dev, before.st_ino)
        ):
            raise PackageError("Payload changed identity before reading: " + item["path"])
        digest = hashlib.sha256()
        count = 0
        info = zip_info(prefix + item["path"], item["bytes"])
        with archive.open(info, "w", force_zip64=True) as destination:
            while chunk := source.read(CHUNK_BYTES):
                count += len(chunk)
                if count > item["bytes"]:
                    raise PackageError("Payload grew while reading: " + item["path"])
                digest.update(chunk)
                destination.write(chunk)
        after = os.fstat(source.fileno())
        if (opened.st_size, opened.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
            raise PackageError("Payload changed while reading: " + item["path"])
    if count != item["bytes"] or digest.hexdigest() != item["sha256"]:
        raise PackageError("Payload byte/hash mismatch: " + item["path"])


def portable_manifest(manifest: dict, files: list[dict], manifest_digest: str) -> bytes:
    result = {
        "schema": PORTABLE_SCHEMA,
        "product": PRODUCT,
        "version": manifest["version"],
        "distribution": "PORTABLE_WINDOWS_X64",
        "sourceInstallerManifest": {"schema": INSTALL_SCHEMA, "sha256": manifest_digest},
        "payloadFiles": files,
        "payloadFileCount": len(files),
        "payloadBytes": sum(item["bytes"] for item in files),
        "generatedPortableFiles": ["install-manifest.json"],
        "uninstallerIncluded": False,
        "installerRegistrationPerformed": False,
        "playerRuntimeStateIncluded": False,
        "scope": "Exact installer payload, independently hash-checked while streaming. "
        "This manifest describes a portable archive, not an installed/uninstalled state.",
    }
    return (json.dumps(result, sort_keys=True, ensure_ascii=False, indent=2) + "\n").encode("utf-8")


def verify_archive(path: Path, prefix: str, files: list[dict], generated: bytes) -> None:
    expected = {prefix + item["path"]: item for item in files}
    expected[prefix + "install-manifest.json"] = {
        "bytes": len(generated), "sha256": hashlib.sha256(generated).hexdigest()
    }
    with zipfile.ZipFile(path, "r") as archive:
        names = archive.namelist()
        if len(names) != len(expected) or set(names) != set(expected):
            raise PackageError("ZIP membership differs from the exact portable manifest")
        for info in archive.infolist():
            if info.is_dir() or info.date_time != FIXED_TIME or (info.external_attr >> 16) != 0o100644:
                raise PackageError("ZIP metadata failed deterministic file-only checks")
            digest = hashlib.sha256()
            count = 0
            with archive.open(info) as source:
                while chunk := source.read(CHUNK_BYTES):
                    count += len(chunk)
                    digest.update(chunk)
            item = expected[info.filename]
            if count != item["bytes"] or digest.hexdigest() != item["sha256"]:
                raise PackageError("ZIP content verification failed: " + info.filename)


def hash_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        while chunk := source.read(CHUNK_BYTES):
            digest.update(chunk)
    return digest.hexdigest()


def package(manifest_path: Path, package_root: Path, output_dir: Path) -> dict:
    if sys.version_info < (3, 13):
        raise PackageError("This packager requires Python 3.13 or newer")
    manifest, files, manifest_digest = load_manifest(manifest_path)
    root = plain_absolute(package_root, directory=True)
    version = manifest["version"]
    prefix = f"HALVETH-Portal-Garden-{version}/"
    output_dir = Path(os.path.abspath(output_dir))
    if not output_dir.exists():
        plain_absolute(output_dir.parent, directory=True)
        output_dir.mkdir()
    output_dir = plain_absolute(output_dir, directory=True)
    if output_dir.is_relative_to(root):
        raise PackageError("Output must be outside the staged package root")
    target = output_dir / f"HALVETH-Portal-Garden-{version}-Windows-portable.zip"
    checksum = target.with_suffix(".zip.sha256")
    if os.path.lexists(target) or os.path.lexists(checksum):
        raise PackageError("Archive or checksum already exists; neither will be overwritten")
    generated = portable_manifest(manifest, files, manifest_digest)
    descriptor, temporary_name = tempfile.mkstemp(prefix="portable-", suffix=".part", dir=output_dir)
    os.close(descriptor)
    temporary = Path(temporary_name)
    checksum_temporary: Path | None = None
    published_checksum = False
    published_archive = False
    try:
        with zipfile.ZipFile(temporary, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
            entries = [(item["path"], item) for item in files] + [("install-manifest.json", None)]
            for name, item in sorted(entries, key=lambda pair: pair[0]):
                if item is None:
                    archive.writestr(zip_info(prefix + name, len(generated)), generated)
                else:
                    stream_payload(archive, root, prefix, item)
        # Reading every member validates ZIP CRC as well as the manifest SHA-256.
        verify_archive(temporary, prefix, files, generated)
        digest = hash_file(temporary)
        descriptor, checksum_name = tempfile.mkstemp(prefix="portable-sha256-", suffix=".part", dir=output_dir)
        checksum_temporary = Path(checksum_name)
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as stream:
            stream.write(f"{digest}  {target.name}\n")
        # Same-directory hard links publish complete bytes atomically and fail if
        # the destination already exists. No replacing rename or race-prone copy.
        os.link(checksum_temporary, checksum)
        published_checksum = True
        os.link(temporary, target)
        published_archive = True
        return {
            "portableExportPassed": True,
            "archive": str(target),
            "archiveBytes": target.stat().st_size,
            "sha256": digest,
            "checksum": str(checksum),
            "sourceInstallerManifestSha256": manifest_digest,
            "payloadFiles": len(files),
            "payloadBytes": sum(item["bytes"] for item in files),
            "zipFiles": len(files) + 1,
            "zipIntegrity": "ALL_MEMBERS_CRC_SIZE_AND_SHA256_VERIFIED",
            "pythonVersion": sys.version.split()[0],
            "zlibVersion": zlib.ZLIB_RUNTIME_VERSION,
        }
    finally:
        # Remove only temporary entries created by this invocation. If publishing
        # the ZIP failed, remove our checksum only when it is still the same file.
        if published_checksum and not published_archive and checksum_temporary is not None:
            if checksum.exists() and os.path.samefile(checksum, checksum_temporary):
                checksum.unlink()
        temporary.unlink(missing_ok=True)
        if checksum_temporary is not None:
            checksum_temporary.unlink(missing_ok=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True, help="Explicit confirmed installer-generated install-manifest.json")
    parser.add_argument("--package-root", type=Path, required=True, help="Exact staged runtime root, normally Builds/Windows")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "dist", help="Existing parent required; defaults to project dist")
    args = parser.parse_args()
    try:
        print(json.dumps(package(args.manifest, args.package_root, args.output_dir), indent=2))
        return 0
    except (PackageError, OSError, ValueError, RuntimeError, zipfile.BadZipFile) as exc:
        print(json.dumps({"portableExportPassed": False, "error": str(exc)}, indent=2), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
