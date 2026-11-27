"""Pinned CC0 assets for VOID.sh; standard library only, no credentials."""
import concurrent.futures
import hashlib
import json
from pathlib import Path
import sys
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "ArtSource"
MANIFEST = json.loads((ART / "ASSETS.json").read_text(encoding="utf-8"))

def target(record):
    p = (ART / record["file"]).resolve()
    if not p.is_relative_to(ART.resolve()):
        raise ValueError("asset path escapes ArtSource")
    return p

def verify(record):
    p = target(record)
    if not p.is_file() or p.stat().st_size != record["size"]:
        return False
    return hashlib.sha256(p.read_bytes()).hexdigest() == record["sha256"]

def download(record):
    if verify(record):
        return
    url = record["url"]
    if not url.startswith("https://dl.polyhaven.org/file/ph-assets/"):
        raise ValueError("unsupported asset origin")
    p = target(record)
    p.parent.mkdir(parents=True, exist_ok=True)
    temporary = p.with_name(p.name + ".part")
    with urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "HALVETH-VOID/0.1"}), timeout=90) as response, temporary.open("wb") as output:
        while block := response.read(1024 * 1024):
            output.write(block)
    if temporary.stat().st_size != record["size"] or hashlib.sha256(temporary.read_bytes()).hexdigest() != record["sha256"]:
        temporary.unlink(missing_ok=True)
        raise ValueError("asset bytes do not match pinned digest: " + record["file"])
    temporary.replace(p)

def main():
    sys.stdout.reconfigure(encoding="utf-8")
    mode = sys.argv[1] if len(sys.argv) == 2 else "verify"
    files = MANIFEST["files"]
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
