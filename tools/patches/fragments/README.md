# patch fragments

source text injected verbatim by `apply_*.py`. these are fragments spliced
into upstream Krita files, not standalone translation units, so they cannot
be compiled or included on their own.

`tests/python/test_patch_sources.py` rejects any 40-line-or-longer snippet
left inside the python.
