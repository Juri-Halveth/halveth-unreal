"""Exact UTF-8 soulseed inputs; no fixed numeric range and no expression eval."""
import argparse
import base64
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOMAIN = b"HALVETH-SOULSEED-v1\x00"


def resolve(text):
    raw = text.encode("utf-8", errors="strict")
    digest = hashlib.sha256(DOMAIN + raw).hexdigest()
    record = {
        "schema": "halveth-soulseed/1.0",
        "recipe": "sha256-domain-utf8-v1",
        "text": text,
        "utf8_base64": base64.b64encode(raw).decode("ascii"),
        "byte_length": len(raw),
        "sha256": digest,
    }
    path = ROOT / "Saved" / "Souls" / (digest + ".json")
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        old = json.loads(path.read_text(encoding="utf-8"))
        if old != record:
            raise ValueError("soulseed address conflict: retained input differs")
    else:
        temporary = path.with_suffix(".tmp")
        temporary.write_text(json.dumps(record, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        restored = json.loads(temporary.read_text(encoding="utf-8"))
        assert restored == record
        assert base64.b64decode(restored["utf8_base64"], validate=True) == raw
        temporary.replace(path)
    return digest


def main():
    parser = argparse.ArgumentParser()
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--text")
    source.add_argument("--file", type=Path)
    args = parser.parse_args()
    # read_bytes/decode preserve a BOM, spaces and line endings as source data.
    text = args.text if args.text is not None else args.file.read_bytes().decode("utf-8", errors="strict")
    print(resolve(text))


if __name__ == "__main__":
    main()
