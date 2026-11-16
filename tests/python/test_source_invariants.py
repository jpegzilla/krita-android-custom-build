"""Stable source invariants that replaced the retired workflow heredoc."""
import ast
from pathlib import Path
import unittest

REPO = Path(__file__).resolve().parents[2]
BUILD_SCRIPT = REPO / "tools/build/build-in-krita-container.sh"
FINGER_SCRIPT = REPO / "tools/patches/apply_finger_raw_motion.py"
FINGER_FRAGMENT = (REPO / "tools/patches/fragments/finger_raw_motion/"
                   "42-pressure-proxy-diagnostics-u.inc")
APPLY_ALL = REPO / "tools/patches/apply_all.py"

# same strings the source-checks workflow asserts
FINGER_BRIDGE_STRINGS = (
    "dispatchTouchEvent(MotionEvent event)",
    "getHistoricalTouchMajor",
    "return super.dispatchTouchEvent(event)",
    "return super.dispatchTouchEvent(event);",
    "fingerRawGestureReset",
    "fingerRawCaptureEnabled",
)


class SourceInvariantTests(unittest.TestCase):
    def test_container_builds_bootstrap_before_install(self):
        script = BUILD_SCRIPT.read_text(encoding="utf-8")
        self.assertIn(
            "engine_only_bootstrap.cpp", script,
            "container build must type-check the studio bootstrap")
        self.assertIn(
            "--target kritaui", script,
            "container build must build kritaui")
        self.assertIn(
            "--target install", script,
            "container build must install")
        self.assertLess(
            script.index("--target kritaui"), script.index("--target install"),
            "kritaui must build before install")

    def test_finger_bridge_strings_present(self):
        raw = "".join(
            path.read_text(encoding="utf-8")
            for path in (FINGER_SCRIPT, FINGER_FRAGMENT))
        for expected in FINGER_BRIDGE_STRINGS:
            with self.subTest(expected=expected):
                self.assertIn(
                    expected, raw,
                    "finger bridge source lost a required string")

    def test_apply_all_order_has_no_duplicates(self):
        tree = ast.parse(APPLY_ALL.read_text(encoding="utf-8"))
        order = next(
            ast.literal_eval(node.value) for node in ast.walk(tree)
            if isinstance(node, ast.Assign)
            and getattr(node.targets[0], "id", None) == "ORDER")
        self.assertEqual(
            len(order), len(set(order)),
            "apply_all ORDER must not repeat entries")


if __name__ == "__main__":
    unittest.main()
