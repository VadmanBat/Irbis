#!/usr/bin/env python3
"""Irbis app mark: snow-leopard paw, plantar, claws up."""

from __future__ import annotations

import math
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SVG_PATH = ROOT / "irbis.svg"

INK = "#0B0E12"
FUR_HI = "#E2EAF0"
FUR_MID = "#B8C6D0"
FUR_LO = "#8795A2"
PAD_HI = "#7B8A96"
PAD_MID = "#3A4650"
PAD_LO = "#151C22"
CLAW_HI = "#A8B0B8"
CLAW_MID = "#3A424C"
CLAW_LO = "#07090C"
CLAW_RIM = "#E8EEF4"
SPOT = "#3A444E"
SPOT_SOFT = "#5A6570"
OUTLINE = 18.0
CLAW_RIM_W = 9.0


def xf(base: tuple[float, float], angle: float, lx: float, ly: float) -> tuple[float, float]:
    """Local claw space: +lx right, +ly toward the tip (up when angle=0)."""
    a = math.radians(angle)
    rx, ry = math.cos(a), math.sin(a)
    ux, uy = math.sin(a), -math.cos(a)
    return (base[0] + lx * rx + ly * ux, base[1] + lx * ry + ly * uy)


def pt(p: tuple[float, float]) -> str:
    return f"{p[0]:.2f},{p[1]:.2f}"


def claw_d(
    base: tuple[float, float],
    length: float,
    width: float,
    angle: float,
    hook: float,
    inset: float = 0.0,
    bury: float = 26.0,
) -> str:
    """Wide saber: stays thick through mid-length, then snaps to a point."""
    length = max(length - inset * 1.02, 16.0)
    width = max(width - inset * 1.05, 10.0)
    hook *= max(1.0 - inset / 50.0, 0.6)
    root = -bury + inset * 0.7
    # Outer edge more convex; inner edge slightly concave (crescent).
    outer = -1.0 if angle <= 0.0 else 1.0

    def side(sign: float, bulge: float, mid: float, near: float) -> list[tuple[float, float]]:
        return [
            xf(base, angle, sign * width * 0.52, root),
            xf(base, angle, sign * width * (0.50 + bulge), length * 0.30),
            xf(base, angle, sign * width * mid, length * 0.64),
            xf(base, angle, sign * width * near + hook * 0.55, length * 0.91),
        ]

    # Outer edge convex, inner edge almost straight → saber, not a leaf.
    if outer < 0:
        left = side(-1.0, 0.12, 0.42, 0.13)
        right = side(1.0, -0.06, 0.18, 0.05)
    else:
        left = side(-1.0, -0.06, 0.18, 0.05)
        right = side(1.0, 0.12, 0.42, 0.13)
    tip = xf(base, angle, hook, length)
    # Rounded root so the claw sits in the toe instead of a hard cut.
    root_c = xf(base, angle, 0.0, root - 6.0)
    return (
        f"M {pt(left[0])} "
        f"C {pt(left[1])} {pt(left[2])} {pt(left[3])} "
        f"L {pt(tip)} "
        f"L {pt(right[3])} "
        f"C {pt(right[2])} {pt(right[1])} {pt(right[0])} "
        f"Q {pt(root_c)} {pt(left[0])} Z"
    )


def claw_hi_d(
    base: tuple[float, float],
    length: float,
    width: float,
    angle: float,
    hook: float,
) -> str:
    a0 = xf(base, angle, -width * 0.22, length * 0.12)
    a1 = xf(base, angle, -width * 0.16, length * 0.48)
    a2 = xf(base, angle, hook * 0.28, length * 0.82)
    b2 = xf(base, angle, hook * 0.04, length * 0.70)
    b1 = xf(base, angle, -width * 0.02, length * 0.42)
    b0 = xf(base, angle, -width * 0.04, length * 0.16)
    return f"M {pt(a0)} L {pt(a1)} L {pt(a2)} L {pt(b2)} L {pt(b1)} L {pt(b0)} Z"


def ell(cx: float, cy: float, rx: float, ry: float, rot: float = 0.0, fill: str = "none", extra: str = "") -> str:
    t = f' transform="rotate({rot:.2f} {cx:.2f} {cy:.2f})"' if abs(rot) > 0.01 else ""
    extra_s = f" {extra}" if extra else ""
    return (
        f'<ellipse cx="{cx:.2f}" cy="{cy:.2f}" rx="{rx:.2f}" ry="{ry:.2f}"'
        f'{t} fill="{fill}"{extra_s}/>'
    )


def palm_d(inset: float = 0.0) -> str:
    s = inset
    return (
        f"M {72 + s},{368 + s * 0.15} "
        f"C {34 + s},{388} {32 + s},{450} {96 + s * 0.4},{488 - s} "
        f"C {148},{500 - s} {200},{508 - s} {256},{508 - s} "
        f"C {312},{508 - s} {364},{500 - s} {416 - s * 0.4},{488 - s} "
        f"C {480 - s},{450} {478 - s},{388} {440 - s},{368 + s * 0.15} "
        f"C {396 - s},{340 + s} {116 + s},{340 + s} {72 + s},{368 + s * 0.15} Z"
    )


def main_pad_d() -> str:
    return (
        "M 124,418 "
        "C 112,378 148,348 204,346 "
        "C 232,345 248,358 256,358 "
        "C 264,358 280,345 308,346 "
        "C 364,348 400,378 388,418 "
        "C 408,458 374,494 324,502 "
        "C 296,508 272,494 256,488 "
        "C 240,494 216,508 188,502 "
        "C 138,494 104,458 124,418 Z"
    )


# Claw bases are planted on the toes (not at the ellipse distal tip),
# so long sabres can point up and still stay inside the square.
TOES = [
    dict(
        cx=120.0, cy=328.0, rx=100.0, ry=122.0, rot=-33.0,
        claw_base=(104.0, 210.0), claw_len=114.0, claw_w=70.0, claw_ang=-26.0, hook=11.0,
        pad=(132.0, 340.0, 44.0, 60.0, -28.0),
    ),
    dict(
        cx=190.0, cy=258.0, rx=80.0, ry=118.0, rot=-8.0,
        claw_base=(188.0, 154.0), claw_len=128.0, claw_w=68.0, claw_ang=-14.0, hook=10.0,
        pad=(190.0, 270.0, 34.0, 62.0, -8.0),
    ),
    dict(
        cx=322.0, cy=258.0, rx=80.0, ry=118.0, rot=8.0,
        claw_base=(324.0, 154.0), claw_len=128.0, claw_w=68.0, claw_ang=14.0, hook=-10.0,
        pad=(322.0, 270.0, 34.0, 62.0, 8.0),
    ),
    dict(
        cx=392.0, cy=328.0, rx=100.0, ry=122.0, rot=33.0,
        claw_base=(408.0, 210.0), claw_len=114.0, claw_w=70.0, claw_ang=26.0, hook=-11.0,
        pad=(380.0, 340.0, 44.0, 60.0, 28.0),
    ),
]


def claw_base(toe: dict) -> tuple[float, float]:
    return toe["claw_base"]


def body_shapes(fill: str, inset: float = 0.0) -> list[str]:
    out = [
        f'<path fill="{fill}" d="{palm_d(inset)}"/>',
        # Bridge so the inner-toe cleft does not punch a hole to the palm.
        ell(256.0, 318.0, 118.0 - inset * 0.35, 96.0 - inset * 0.25, 0.0, fill),
    ]
    for t in TOES:
        out.append(ell(t["cx"], t["cy"], max(t["rx"] - inset, 8.0), max(t["ry"] - inset, 8.0), t["rot"], fill))
        bx, by = t["claw_base"]
        out.append(ell(bx, by, max(t["claw_w"] * 0.38 - inset * 0.25, 6.0), max(t["claw_w"] * 0.28 - inset * 0.2, 5.0), t["claw_ang"], fill))
    return out


def claw_shapes(fill: str, inset: float = 0.0, bury: float = 26.0) -> list[str]:
    out = []
    for t in TOES:
        d = claw_d(claw_base(t), t["claw_len"], t["claw_w"], t["claw_ang"], t["hook"], inset, bury)
        out.append(f'<path fill="{fill}" d="{d}"/>')
    return out


def spots() -> list[str]:
    marks = [
        (64, 300, 12, 9, -40), (80, 318, 7.5, 6, 18), (56, 326, 6, 5, -8),
        (92, 284, 6.5, 5, 28),
        (158, 210, 8, 6, -18), (172, 198, 5, 4, 12), (150, 224, 6.5, 5, 36),
        (256, 228, 7, 5.5, 0), (244, 214, 4.5, 3.5, -28), (268, 214, 4.5, 3.5, 28),
        (354, 210, 8, 6, 18), (340, 198, 5, 4, -12), (362, 224, 6.5, 5, -36),
        (448, 300, 12, 9, 40), (432, 318, 7.5, 6, -18), (456, 326, 6, 5, 8),
        (420, 284, 6.5, 5, -28),
        (92, 420, 13, 10, -22), (108, 444, 8, 6, 14), (80, 452, 6.5, 5, -6),
        (420, 420, 13, 10, 22), (404, 444, 8, 6, -14), (432, 452, 6.5, 5, 6),
        (256, 488, 8, 5.5, 0), (232, 494, 6, 4.5, -18), (280, 494, 6, 4.5, 18),
        (72, 368, 7, 5.5, 32), (440, 368, 7, 5.5, -32),
        (186, 312, 8, 6.5, 16), (326, 312, 8, 6.5, -16),
    ]
    out = [ell(x, y, rx, ry, rot, SPOT, 'opacity=".9"') for x, y, rx, ry, rot in marks]
    for x, y, rx, ry, rot in [(256, 312, 6, 5, 0), (200, 360, 7, 5.5, 10), (312, 360, 7, 5.5, -10)]:
        out.append(ell(x, y, rx, ry, rot, SPOT_SOFT, 'opacity=".65"'))
    return out


def pads() -> list[str]:
    out = []
    for t in TOES:
        cx, cy, rx, ry, rot = t["pad"]
        out.append(ell(cx, cy, rx + 3.2, ry + 3.2, rot, PAD_LO))
        out.append(ell(cx, cy, rx, ry, rot, "url(#pad)"))
        out.append(ell(cx - 6, cy - 14, rx * 0.46, ry * 0.22, rot, PAD_HI, 'opacity=".42"'))
        out.append(ell(cx + 8, cy + 18, rx * 0.50, ry * 0.24, rot, PAD_LO, 'opacity=".26"'))
    out.append(f'<path fill="{PAD_LO}" d="{main_pad_d()}"/>')
    out.append(
        '<g transform="translate(256 430) scale(0.94) translate(-256 -430)">'
        f'<path fill="url(#mainpad)" d="{main_pad_d()}"/>'
        "</g>"
    )
    out.append(
        '<ellipse cx="214" cy="400" rx="50" ry="26" fill="#7A8894" opacity=".38" '
        'transform="rotate(-12 214 400)"/>'
    )
    out.append(
        '<ellipse cx="300" cy="462" rx="68" ry="20" fill="#151C22" opacity=".22" '
        'transform="rotate(8 300 462)"/>'
    )
    return out


def clefts() -> list[str]:
    return [
        ell(150, 268, 20, 14, -26, FUR_LO, 'opacity=".32"'),
        ell(256, 248, 22, 28, 0, FUR_LO, 'opacity=".40"'),
        ell(362, 268, 20, 14, 26, FUR_LO, 'opacity=".32"'),
        ell(256, 200, 10, 18, 0, FUR_LO, 'opacity=".35"'),
        ell(190, 188, 26, 16, -8, FUR_HI, 'opacity=".38"'),
        ell(322, 188, 26, 16, 8, FUR_HI, 'opacity=".38"'),
        ell(100, 288, 24, 14, -34, FUR_HI, 'opacity=".30"'),
        ell(412, 288, 24, 14, 34, FUR_HI, 'opacity=".30"'),
        ell(256, 478, 118, 34, 0, FUR_LO, 'opacity=".20"'),
    ]


def build() -> str:
    parts: list[str] = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<svg xmlns="http://www.w3.org/2000/svg"',
        '     viewBox="0 0 512 512"',
        '     role="img"',
        '     aria-label="Irbis">',
        "  <title>Irbis</title>",
        "  <desc>Snow leopard paw, plantar, claws up. No background.</desc>",
        "  <defs>",
        '    <radialGradient id="fur" cx="236" cy="240" r="300" gradientUnits="userSpaceOnUse">',
        f'      <stop offset="0%" stop-color="{FUR_HI}"/>',
        f'      <stop offset="50%" stop-color="{FUR_MID}"/>',
        f'      <stop offset="100%" stop-color="{FUR_LO}"/>',
        "    </radialGradient>",
        '    <radialGradient id="pad" cx="38%" cy="32%" r="72%">',
        f'      <stop offset="0%" stop-color="{PAD_HI}"/>',
        f'      <stop offset="55%" stop-color="{PAD_MID}"/>',
        f'      <stop offset="100%" stop-color="{PAD_LO}"/>',
        "    </radialGradient>",
        '    <radialGradient id="mainpad" cx="36%" cy="30%" r="78%">',
        f'      <stop offset="0%" stop-color="{PAD_HI}"/>',
        f'      <stop offset="42%" stop-color="{PAD_MID}"/>',
        f'      <stop offset="100%" stop-color="{PAD_LO}"/>',
        "    </radialGradient>",
        '    <linearGradient id="claw" x1="22%" y1="8%" x2="78%" y2="100%">',
        f'      <stop offset="0%" stop-color="{CLAW_MID}"/>',
        f'      <stop offset="40%" stop-color="{CLAW_LO}"/>',
        f'      <stop offset="100%" stop-color="{CLAW_LO}"/>',
        "    </linearGradient>",
        "  </defs>",
        "",
        '  <g id="ink">',
    ]
    parts += [f"    {s}" for s in body_shapes(INK, 0.0)]
    parts += [f"    {s}" for s in claw_shapes(INK, 0.0, bury=30.0)]
    parts += [
        "  </g>",
        "",
        # Pale claw rim on top of the ink claws; fur then covers the roots
        # so only the free edges (sides + tips) keep the keyline.
        '  <g id="claw-rim">',
    ]
    parts += [f"    {s}" for s in claw_shapes(CLAW_RIM, 0.0, bury=30.0)]
    parts += [
        "  </g>",
        "",
        '  <g id="fur">',
    ]
    parts += [f"    {s}" for s in body_shapes("url(#fur)", OUTLINE * 0.58)]
    parts += [f"    {s}" for s in clefts()]
    parts += [f"    {s}" for s in spots()]
    parts += [
        "  </g>",
        "",
        '  <g id="pads">',
    ]
    parts += [f"    {s}" for s in pads()]
    parts += [
        "  </g>",
        "",
        '  <g id="claws">',
    ]
    parts += [f"    {s}" for s in claw_shapes("url(#claw)", CLAW_RIM_W, bury=26.0)]
    for t in TOES:
        d = claw_hi_d(claw_base(t), t["claw_len"], t["claw_w"], t["claw_ang"], t["hook"])
        parts.append(f'    <path fill="{CLAW_HI}" opacity=".88" d="{d}"/>')
    parts += [
        "  </g>",
        "</svg>",
        "",
    ]
    return "\n".join(parts)


def _export_rasters() -> None:
    import struct
    import subprocess

    inkscape = Path(r"C:\Program Files\Inkscape\bin\inkscape.exe")
    if not inkscape.exists():
        print("inkscape not found; skip png/ico")
        return
    preview = ROOT / "_preview"
    preview.mkdir(exist_ok=True)
    sizes = [16, 24, 32, 48, 64, 128, 256, 512]
    for s in sizes:
        dest = preview / f"ico-{s}.png"
        subprocess.run(
            [
                str(inkscape),
                str(SVG_PATH),
                "--export-type=png",
                f"--export-filename={dest}",
                f"--export-width={s}",
                "--export-background-opacity=0",
            ],
            check=True,
        )
    png_512 = preview / "ico-512.png"
    (ROOT / "irbis.png").write_bytes(png_512.read_bytes())

    blobs = [(s, (preview / f"ico-{s}.png").read_bytes()) for s in sizes if s <= 256]
    offset = 6 + 16 * len(blobs)
    header = struct.pack("<HHH", 0, 1, len(blobs))
    entries = b""
    payload = b""
    for s, data in blobs:
        bw = 0 if s >= 256 else s
        entries += struct.pack("<BBBBHHII", bw, bw, 0, 0, 1, 32, len(data), offset)
        payload += data
        offset += len(data)
    (ROOT / "irbis.ico").write_bytes(header + entries + payload)
    print("wrote irbis.png and irbis.ico")


def main() -> None:
    SVG_PATH.write_text(build(), encoding="utf-8")
    print(f"wrote {SVG_PATH}")
    _export_rasters()



if __name__ == "__main__":
    main()

