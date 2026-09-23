"""Check portable source-delivery invariants; this does not compile or run Unreal."""
from __future__ import annotations

import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]


def validate(root: Path = ROOT) -> list[str]:
    errors: list[str] = []
    project = json.loads((root / "HALVETHRealms.uproject").read_text(encoding="utf-8"))
    if project.get("EngineAssociation") != "5.8":
        errors.append("Expected explicitly bound UE 5.8 major/minor association")
    modules = {m["Name"]: m["Type"] for m in project["Modules"]}
    if modules != {"HALVETHRealms": "Runtime", "HALVETHRealmsEditor": "Editor"}:
        errors.append("Runtime and editor modules must remain distinct")
    for name in modules:
        if not (root / "Source" / name / (name + ".Build.cs")).is_file():
            errors.append(f"Missing build module {name}")
    runtime_build = (root / "Source/HALVETHRealms/HALVETHRealms.Build.cs").read_text()
    if '"UnrealEd"' in runtime_build:
        errors.append("Runtime must not depend on editor tooling")
    for header in (root / "Source").rglob("*.h"):
        text = header.read_text(encoding="utf-8")
        includes = re.findall(r'^#include\s+[<"]([^>"\n]+)', text, re.M)
        generated = [item for item in includes if item.endswith(".generated.h")]
        if generated and includes[-1] != generated[-1]:
            errors.append(f"Generated UHT header must be last include: {header.name}")
    inputs = (root / "Config/DefaultInput.ini").read_text()
    character = (root / "Source/HALVETHRealms/Private/HALVETHCharacter.cpp").read_text()
    for action in re.findall(r'ActionName="([^"]+)"', inputs):
        if f'TEXT("{action}")' not in character:
            errors.append(f"Configured input lacks character binding: {action}")
    for rel in ["LICENSE", "THIRD-PARTY-NOTICES.md", "Source/HALVETHRealms/Public/RealmLayout.h"]:
        if not (root / rel).is_file():
            errors.append(f"Missing delivery file {rel}")
    return errors


if __name__ == "__main__":
    found = validate()
    print(json.dumps({"schema": 1, "sourceStructurePassed": not found,
                      "unrealCompilationVerified": False, "runtimeVerified": False,
                      "errors": found}, indent=2))
    sys.exit(bool(found))
