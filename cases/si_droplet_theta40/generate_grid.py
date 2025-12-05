#!/usr/bin/env python3
import math
from pathlib import Path

VOLUME_PL = 3.50         # droplet volume in pL
# 1 pL = 1e-15 m^3; choose dx ~1 µm to keep O(10^4) particles for this volume
DX = 1.0e-6              # particle spacing (m)
IMPACT_SPEED = 1.0       # initial downward speed (m/s)
SUBSTRATE_Z = 0.0

def main():
    root = Path(__file__).resolve().parent
    out = root / "si_droplet_theta40.grid"

    # 1 pL = 1e-15 m^3
    volume_m3 = VOLUME_PL * 1e-15
    radius = (3.0 * volume_m3 / (4.0 * math.pi)) ** (1.0 / 3.0)
    center = (0.0, 0.0, SUBSTRATE_Z + radius + 2.0 * DX)

    points = []
    # simple cubic lattice inside a sphere
    n = int(math.ceil(radius / DX))
    for ix in range(-n, n + 1):
        x = center[0] + ix * DX
        for iy in range(-n, n + 1):
            y = center[1] + iy * DX
            for iz in range(-n, n + 1):
                z = center[2] + iz * DX
                if (x - center[0]) ** 2 + (y - center[1]) ** 2 + (z - center[2]) ** 2 <= radius ** 2:
                    points.append((x, y, z))

    with out.open("w", encoding="utf-8") as f:
        f.write("0\n")
        f.write(f"{len(points)}\n")
        for (x, y, z) in points:
            f.write(f"0 {x:.9e} {y:.9e} {z:.9e} 0.0 0.0 {-IMPACT_SPEED:.6f} 0.0 0.0\n")

    print(f"Wrote {len(points)} particles to {out}")
    print(f"Radius {radius*1e6:.2f} um, dx {DX*1e6:.2f} um")

if __name__ == "__main__":
    main()
