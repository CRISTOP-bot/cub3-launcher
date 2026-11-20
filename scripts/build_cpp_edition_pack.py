#!/usr/bin/env python3
"""Rebuild CUB3's small original Minecraft 26.3 title-screen resource pack."""
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

root = Path(__file__).parent.parent / "resources" / "cpp_edition"
out = Path(__file__).parent.parent / "resources" / "CUB3CppEdition.zip"
with ZipFile(out, "w", compression=ZIP_DEFLATED) as archive:
    for path in sorted(root.rglob("*")):
        if path.is_file():
            archive.write(path, path.relative_to(root).as_posix())
print(f"Wrote {out} ({out.stat().st_size} bytes)")
