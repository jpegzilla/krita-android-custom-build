"""Paths and helpers shared by the Krita source patch scripts."""
from pathlib import Path

PATCH_DIR = Path(__file__).resolve().parent
REPO = PATCH_DIR.parents[1]
CORE = REPO / "mobile_studio" / "include" / "mobile_studio"
SRC = REPO / "mobile_studio" / "src"


def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected 1 anchor, got {count}")
    return text.replace(old, new, 1)
