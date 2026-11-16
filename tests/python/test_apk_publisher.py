"""APK publisher asset matching; no network or gh calls."""
import importlib.util
from pathlib import Path
import sys
import unittest

BUILD_DIR = Path(__file__).resolve().parents[2] / "tools" / "build"
MODULE_PATH = BUILD_DIR / "krita_apk_publisher.py"


def load_module():
    if str(BUILD_DIR) not in sys.path:
        sys.path.insert(0, str(BUILD_DIR))
    spec = importlib.util.spec_from_file_location(
        "krita_apk_publisher", MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class AssetMatchesTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.module = load_module()

    def test_exact_size_without_digest_matches(self):
        self.assertTrue(
            self.module.asset_matches({"size": 100}, 100, "a" * 64),
            "exact size without a digest must match")

    def test_size_mismatch_rejected(self):
        asset = {"size": 99, "digest": "sha256:" + "a" * 64}
        self.assertFalse(
            self.module.asset_matches(asset, 100, "a" * 64),
            "size mismatch must reject the asset")

    def test_digest_mismatch_rejected(self):
        asset = {"size": 100, "digest": "sha256:" + "b" * 64}
        self.assertFalse(
            self.module.asset_matches(asset, 100, "a" * 64),
            "sha256 mismatch must reject the asset")

    def test_matching_digest_accepted(self):
        asset = {"size": 100, "digest": "sha256:" + "A" * 64}
        self.assertTrue(
            self.module.asset_matches(asset, 100, "a" * 64),
            "matching sha256 must accept the asset")

    def test_none_asset_rejected(self):
        self.assertFalse(
            self.module.asset_matches(None, 100, "a" * 64),
            "missing asset must not match")


if __name__ == "__main__":
    unittest.main()
