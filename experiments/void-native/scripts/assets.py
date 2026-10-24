"""Pinned CC0 assets for VOID.sh; standard library only, no credentials."""
import concurrent.futures
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "ArtSource"
MANIFEST = json.loads((ART / "ASSETS.json").read_text(encoding="utf-8"))
MAX_ASSET_BYTES = 200_000_000
CHUNK_SIZE = 1024 * 1024

def target(record):
    p = (ART / record["file"]).resolve()
    if not p.is_relative_to(ART.resolve()):
        raise ValueError("asset path escapes ArtSource")
    return p

def admitted_size(record):
    size = record["size"]
    if type(size) is not int or not 0 <= size <= MAX_ASSET_BYTES:
        raise ValueError("invalid asset byte budget")
    if "expected_size" in record and (type(record["expected_size"]) is not int or record["expected_size"] != size):
        raise ValueError("conflicting asset byte budgets")
    return size

def admit_manifest(manifest):
    cap = manifest["byte_cap"]
    if type(cap) is not int or not 0 <= cap <= MAX_ASSET_BYTES:
        raise ValueError("invalid aggregate asset byte cap")
    if not isinstance(manifest["files"], list):
        raise ValueError("asset files must be a list")
    files = tuple(dict(record) for record in manifest["files"])
    total = 0
    destinations = set()
    for record in files:
        total += admitted_size(record)
        if total > cap:
            raise ValueError("aggregate asset byte cap exceeded")
        destination = target(record)
        if destination in destinations:
            raise ValueError("duplicate asset destination")
        destinations.add(destination)
    return files

def verify(record):
    size = admitted_size(record)
    p = target(record)
    if not p.is_file() or p.stat().st_size != size:
        return False
    digest = hashlib.sha256()
    count = 0
    with p.open("rb") as source:
        while block := source.read(min(CHUNK_SIZE, size - count + 1)):
            count += len(block)
            if count > size:
                return False
            digest.update(block)
    return count == size and digest.hexdigest() == record["sha256"]

def download(record):
    size = admitted_size(record)
    if verify(record):
        return
    url = record["url"]
    if not url.startswith("https://dl.polyhaven.org/file/ph-assets/"):
        raise ValueError("unsupported asset origin")
    p = target(record)
    p.parent.mkdir(parents=True, exist_ok=True)
    output_file = tempfile.NamedTemporaryFile(prefix=p.name + ".", suffix=".part", dir=p.parent, delete=False)
    temporary = Path(output_file.name)
    try:
        with output_file as output:
            digest = hashlib.sha256()
            count = 0
            with urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "HALVETH-VOID/0.1"}), timeout=90) as response:
                while block := response.read(min(CHUNK_SIZE, size - count + 1)):
                    if len(block) > size - count:
                        raise ValueError("asset exceeds pinned byte budget: " + record["file"])
                    if output.write(block) != len(block):
                        raise OSError("incomplete asset write")
                    count += len(block)
                    digest.update(block)
            output.flush()
        if count != size or temporary.stat().st_size != size or digest.hexdigest() != record["sha256"]:
            raise ValueError("asset bytes do not match pinned digest: " + record["file"])
        temporary.replace(p)
    finally:
        temporary.unlink(missing_ok=True)

def main():
    sys.stdout.reconfigure(encoding="utf-8")
    mode = sys.argv[1] if len(sys.argv) == 2 else "verify"
    files = MANIFEST["files"]
    if mode in {"verify", "download"}:
        files = admit_manifest(MANIFEST)
    if mode == "download":
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            list(pool.map(download, files))
    if mode in {"verify", "download"}:
        failures = [r["file"] for r in files if not verify(r)]
        if failures:
            print("Fehlende oder geaenderte Quellen: " + ", ".join(failures), file=sys.stderr)
            print("bash VOID.sh download", file=sys.stderr)
            return 2
        print(f"ASSETS PASS: {len(files)} Dateien, {sum(r['size'] for r in files)} Bytes.")
        return 0
    if mode == "qa":
        report = json.loads((ROOT / "Saved" / "VOID-QA.json").read_text(encoding="utf-8-sig"))
        print(json.dumps(report, ensure_ascii=False, indent=2))
        return 0 if report["passed"] else 3
    raise ValueError("unknown mode")

if __name__ == "__main__":
    raise SystemExit(main())
