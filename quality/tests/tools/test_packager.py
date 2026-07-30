from pathlib import Path

import pytest

from tools.package.packager import FDT_MAGIC, _compact_qemu_dtb


def _dtb_bytes(total_size: int, padded_size: int) -> bytes:
    header = bytearray(40)
    header[0:4] = FDT_MAGIC.to_bytes(4, "big")
    header[4:8] = padded_size.to_bytes(4, "big")
    header[8:12] = (56).to_bytes(4, "big")
    header[12:16] = (60).to_bytes(4, "big")
    header[16:20] = (40).to_bytes(4, "big")
    header[32:36] = (total_size - 60).to_bytes(4, "big")
    header[36:40] = (4).to_bytes(4, "big")
    return bytes(header) + bytes(padded_size - len(header))


def test_compact_qemu_dtb_removes_padding(tmp_path: Path) -> None:
    dtb = tmp_path / "hw.dtb"
    dtb.write_bytes(_dtb_bytes(total_size=64, padded_size=1024))

    _compact_qemu_dtb(dtb)

    assert len(dtb.read_bytes()) == 64


@pytest.mark.parametrize(
    "contents",
    [
        b"too short",
        b"BAD!" + (40).to_bytes(4, "big") + bytes(32),
        FDT_MAGIC.to_bytes(4, "big") + (128).to_bytes(4, "big") + bytes(32),
    ],
)
def test_compact_qemu_dtb_rejects_invalid_blob(tmp_path: Path, contents: bytes) -> None:
    dtb = tmp_path / "hw.dtb"
    dtb.write_bytes(contents)

    with pytest.raises(RuntimeError, match="DTB"):
        _compact_qemu_dtb(dtb)
