"""Build cache fingerprint and snapshot invariants."""
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[2]
MODULE_PATH = REPO / "tools" / "build" / "krita_build_cache.py"


def load_module():
    spec = importlib.util.spec_from_file_location(
        "krita_build_cache", MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class BuildCacheTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if shutil.which("git") is None:
            raise unittest.SkipTest(
                "git is not installed; skipping build cache tests")
        cls.module = load_module()

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.repo = Path(self.tmp.name)
        self.git("init", "-q")
        (self.repo / "app.cpp").write_text(
            "int main() { return 0; }\n", encoding="utf-8")
        (self.repo / "tools/build").mkdir(parents=True)
        self.publisher = self.repo / "tools/build/krita_apk_publisher.py"
        self.publisher.write_text("VERSION = 1\n", encoding="utf-8")

    def git(self, *args):
        return subprocess.run(["git", "-C", str(self.repo), *args],
                              check=True, capture_output=True, text=True)

    def test_untracked_app_file_changes_fingerprint(self):
        before = self.module.fingerprint(self.repo)
        (self.repo / "studio_extra.cpp").write_text(
            "// extra\n", encoding="utf-8")
        self.assertNotEqual(
            before, self.module.fingerprint(self.repo),
            "untracked app source must change the fingerprint")

    def test_non_app_untracked_files_are_ignored(self):
        before = self.module.fingerprint(self.repo)
        for rel in ("notes.md", "tests/test_extra.py",
                    ".github/workflows/extra.yml"):
            path = self.repo / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("ignored\n", encoding="utf-8")
        self.assertEqual(
            before, self.module.fingerprint(self.repo),
            "md, tests and workflow files must not change the fingerprint")

    def test_publisher_edit_is_ignored(self):
        before = self.module.fingerprint(self.repo)
        self.publisher.write_text("VERSION = 2\n", encoding="utf-8")
        self.assertEqual(
            before, self.module.fingerprint(self.repo),
            "publisher plumbing edit must not change the fingerprint")

    def test_snapshot_includes_untracked_generated_header(self):
        header = self.repo / "libs/ui/standalone_host.h"
        header.parent.mkdir(parents=True)
        header.write_text("#pragma once\n", encoding="utf-8")
        manifest = self.repo / "build/patch-manifest.json"
        with contextlib.redirect_stdout(io.StringIO()):
            self.module.snapshot(self.repo, manifest)
        data = json.loads(manifest.read_text(encoding="utf-8"))
        self.assertEqual(data["version"], 1)
        self.assertIn(
            "libs/ui/standalone_host.h", data["files"],
            "snapshot must record untracked generated headers")
        self.assertEqual(
            data["files"]["libs/ui/standalone_host.h"]["sha"],
            self.module.sha(header))


if __name__ == "__main__":
    unittest.main()
