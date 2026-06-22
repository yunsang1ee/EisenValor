#!/usr/bin/env python3
"""Convert an OBJ mesh to EisenValor's static EVMH mesh format for debug display."""

from __future__ import annotations

import argparse
import math
import struct
from array import array
from pathlib import Path


ASSET_GUID = bytes.fromhex("4d41504f424a44454255474d45534830")  # MAPOBJDEBUGMESH0
VERSION = 2
HEADER_SIZE = 64
CHUNK_ENTRY_SIZE = 32


def _align(value: int, alignment: int = 16) -> int:
    return (value + alignment - 1) // alignment * alignment


def _pack_chunk_entry(chunk_type: bytes, offset: int, size: int, version: int = 1) -> bytes:
    return struct.pack("<4sIQQQ", chunk_type, version, offset, size, size)


def _parse_face_index(token: str, vertex_count: int) -> int:
    raw = token.split("/", 1)[0]
    if not raw:
        raise ValueError(f"Invalid OBJ face token: {token!r}")

    index = int(raw)
    if index > 0:
        return index - 1
    if index < 0:
        return vertex_count + index
    raise ValueError("OBJ vertex indices are 1-based; index 0 is invalid")


def scan_obj(path: Path) -> dict[str, object]:
    vertices = 0
    faces = 0
    triangles = 0
    bounds_min = [math.inf, math.inf, math.inf]
    bounds_max = [-math.inf, -math.inf, -math.inf]

    with path.open("r", encoding="utf-8", errors="ignore", newline="") as file:
        for line in file:
            if line.startswith("v "):
                parts = line.split()
                if len(parts) >= 4:
                    xyz = (float(parts[1]), float(parts[2]), float(parts[3]))
                    vertices += 1
                    for axis in range(3):
                        bounds_min[axis] = min(bounds_min[axis], xyz[axis])
                        bounds_max[axis] = max(bounds_max[axis], xyz[axis])
            elif line.startswith("f "):
                count = len(line.split()) - 1
                if count >= 3:
                    faces += 1
                    triangles += count - 2

    return {
        "vertices": vertices,
        "faces": faces,
        "triangles": triangles,
        "bounds_min": bounds_min,
        "bounds_max": bounds_max,
    }


def load_obj(path: Path, face_step: int = 1) -> tuple[list[tuple[float, float, float]], list[int], dict[str, object]]:
    vertices: list[tuple[float, float, float]] = []
    indices: list[int] = []
    faces_read = 0
    faces_used = 0
    triangles = 0
    bounds_min = [math.inf, math.inf, math.inf]
    bounds_max = [-math.inf, -math.inf, -math.inf]

    with path.open("r", encoding="utf-8", errors="ignore", newline="") as file:
        for line in file:
            if line.startswith("v "):
                parts = line.split()
                if len(parts) < 4:
                    continue
                xyz = (float(parts[1]), float(parts[2]), float(parts[3]))
                vertices.append(xyz)
                for axis in range(3):
                    bounds_min[axis] = min(bounds_min[axis], xyz[axis])
                    bounds_max[axis] = max(bounds_max[axis], xyz[axis])
            elif line.startswith("f "):
                face_tokens = line.split()[1:]
                if len(face_tokens) < 3:
                    continue
                faces_read += 1
                if face_step > 1 and ((faces_read - 1) % face_step) != 0:
                    continue

                face = [_parse_face_index(token, len(vertices)) for token in face_tokens]
                first = face[0]
                for i in range(1, len(face) - 1):
                    indices.extend((first, face[i], face[i + 1]))
                    triangles += 1
                faces_used += 1

    stats = {
        "vertices": len(vertices),
        "faces_read": faces_read,
        "faces_used": faces_used,
        "triangles": triangles,
        "bounds_min": bounds_min,
        "bounds_max": bounds_max,
    }
    return vertices, indices, stats


def compact_mesh(
    vertices: list[tuple[float, float, float]], indices: list[int]
) -> tuple[list[tuple[float, float, float]], list[int]]:
    remap: dict[int, int] = {}
    compact_vertices: list[tuple[float, float, float]] = []
    compact_indices: list[int] = []

    for index in indices:
        new_index = remap.get(index)
        if new_index is None:
            new_index = len(compact_vertices)
            remap[index] = new_index
            compact_vertices.append(vertices[index])
        compact_indices.append(new_index)

    return compact_vertices, compact_indices


def build_vertex_payload(vertices: list[tuple[float, float, float]]) -> bytes:
    payload = bytearray(len(vertices) * 48)
    offset = 0
    for x, y, z in vertices:
        # position.xyz, normal.xyz, tangent.xyzw, uv.xy
        struct.pack_into("<ffffffffffff", payload, offset, x, y, z, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
        offset += 48
    return bytes(payload)


def build_index_payload(indices: list[int]) -> bytes:
    payload = bytearray(8 + len(indices) * 4)
    struct.pack_into("<II", payload, 0, 32, len(indices))
    index_array = array("I", indices)
    if index_array.itemsize != 4:
        raise RuntimeError("Unexpected uint index array size")
    if struct.pack("<I", 1) != b"\1\0\0\0":
        index_array.byteswap()
    payload[8:] = index_array.tobytes()
    return bytes(payload)


def build_submesh_payload(index_count: int, bounds_min: list[float], bounds_max: list[float]) -> bytes:
    payload = bytearray(4 + 36)
    struct.pack_into("<I", payload, 0, 1)
    struct.pack_into("<IIIffffff", payload, 4, 0, index_count, 0, *bounds_min, *bounds_max)
    return bytes(payload)


def build_bounds_payload(bounds_min: list[float], bounds_max: list[float]) -> bytes:
    center = [(bounds_min[i] + bounds_max[i]) * 0.5 for i in range(3)]
    radius = math.sqrt(sum((bounds_max[i] - center[i]) ** 2 for i in range(3)))
    return struct.pack("<ffffffffff", *bounds_min, *bounds_max, *center, radius)


def write_evmesh(output_path: Path, vertices: list[tuple[float, float, float]], indices: list[int], stats: dict[str, object]) -> None:
    if not vertices or not indices:
        raise ValueError("OBJ conversion produced an empty mesh")
    if len(indices) > 0xFFFFFFFF:
        raise ValueError("Index count exceeds EVMH 32-bit index count limit")

    bounds_min = stats["bounds_min"]
    bounds_max = stats["bounds_max"]
    assert isinstance(bounds_min, list)
    assert isinstance(bounds_max, list)

    chunks: list[tuple[bytes, bytes]] = [
        (b"VERT", build_vertex_payload(vertices)),
        (b"INDX", build_index_payload(indices)),
        (b"SUBM", build_submesh_payload(len(indices), bounds_min, bounds_max)),
        (b"BNDS", build_bounds_payload(bounds_min, bounds_max)),
        (b"DEPS", struct.pack("<I", 0)),
    ]

    chunk_table_offset = HEADER_SIZE
    data_offset = _align(chunk_table_offset + len(chunks) * CHUNK_ENTRY_SIZE)
    entries = bytearray()
    payload = bytearray()
    cursor = data_offset

    for chunk_type, chunk_payload in chunks:
        cursor = _align(cursor)
        if len(payload) < cursor - data_offset:
            payload.extend(b"\0" * (cursor - data_offset - len(payload)))
        entries.extend(_pack_chunk_entry(chunk_type, cursor, len(chunk_payload)))
        payload.extend(chunk_payload)
        cursor += len(chunk_payload)

    file_size = data_offset + len(payload)
    header = struct.pack(
        "<4sIII16sQI20s",
        b"EVMH",
        VERSION,
        HEADER_SIZE,
        0,
        ASSET_GUID,
        file_size,
        len(chunks),
        b"\0" * 20,
    )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("wb") as file:
        file.write(header)
        file.write(entries)
        file.write(b"\0" * (data_offset - HEADER_SIZE - len(entries)))
        file.write(payload)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="Input OBJ path")
    parser.add_argument("output", type=Path, nargs="?", help="Output .evmesh path")
    parser.add_argument("--stats-only", action="store_true", help="Only print OBJ statistics")
    parser.add_argument("--face-step", type=int, default=1, help="Use every Nth OBJ face for a lighter debug mesh")
    parser.add_argument("--compact-unused", action="store_true", help="Drop vertices not referenced by exported faces")
    args = parser.parse_args()

    if args.face_step < 1:
        raise ValueError("--face-step must be >= 1")

    if args.stats_only:
        stats = scan_obj(args.input)
        print(stats)
        return

    if args.output is None:
        raise ValueError("Output path is required unless --stats-only is used")

    vertices, indices, stats = load_obj(args.input, args.face_step)
    if args.compact_unused:
        vertices, indices = compact_mesh(vertices, indices)
        stats["vertices"] = len(vertices)
    write_evmesh(args.output, vertices, indices, stats)
    print(
        f"Exported {stats['faces_used']} faces / {stats['triangles']} triangles "
        f"from {stats['faces_read']} OBJ faces to {args.output}"
    )
    print(f"vertices={stats['vertices']} indices={len(indices)}")
    print(f"bounds_min={stats['bounds_min']} bounds_max={stats['bounds_max']}")


if __name__ == "__main__":
    main()
