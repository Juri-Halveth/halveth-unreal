import importlib.util
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("validate_project", ROOT / "scripts/validate_project.py")
VALIDATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VALIDATOR)


class DeliveryContractTests(unittest.TestCase):
    def test_actual_source_contract(self):
        self.assertEqual(VALIDATOR.validate(), [])

    def test_editor_dependency_is_rejected_in_runtime(self):
        with tempfile.TemporaryDirectory() as tmp:
            replica = Path(tmp)
            for folder in ["Source", "Config"]:
                shutil.copytree(ROOT / folder, replica / folder)
            for item in ["HALVETHRealms.uproject", "LICENSE", "THIRD-PARTY-NOTICES.md"]:
                shutil.copy2(ROOT / item, replica / item)
            target = replica / "Source/HALVETHRealms/HALVETHRealms.Build.cs"
            target.write_text(target.read_text() + '\n// "UnrealEd"\n')
            self.assertIn("Runtime must not depend on editor tooling", VALIDATOR.validate(replica))

    def test_unbound_input_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            replica = Path(tmp)
            for folder in ["Source", "Config"]:
                shutil.copytree(ROOT / folder, replica / folder)
            for item in ["HALVETHRealms.uproject", "LICENSE", "THIRD-PARTY-NOTICES.md"]:
                shutil.copy2(ROOT / item, replica / item)
            with (replica / "Config/DefaultInput.ini").open("a") as stream:
                stream.write('\n+ActionMappings=(ActionName="UnboundTest",Key=F12)\n')
            self.assertTrue(any("UnboundTest" in error for error in VALIDATOR.validate(replica)))


if __name__ == "__main__":
    unittest.main()
