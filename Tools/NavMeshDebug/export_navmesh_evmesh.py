import argparse
import math
import os
import struct
from pathlib import Path


NAVMESH_SET_HEADER_SIZE = 40
NAVMESH_TILE_HEADER_SIZE = 8
MESH_HEADER_SIZE = 100
DT_VERTS_PER_POLYGON = 6
DT_POLY_SIZE = 32
DT_POLY_DETAIL_SIZE = 12
DT_LINK_SIZE = 12
DT_BV_NODE_SIZE = 16
DT_OFF_MESH_CONNECTION_SIZE = 36
ASSET_HEADER_SIZE = 64
CHUNK_ENTRY_SIZE = 32


def align4(value: int) -> int:
    return (value + 3) & ~3


def read_i32(blob: bytes, offset: int) -> int:
    return struct.unpack_from("<i", blob, offset)[0]


def read_u32(blob: bytes, offset: int) -> int:
    return struct.unpack_from("<I", blob, offset)[0]


def read_f32(blob: bytes, offset: int) -> float:
    return struct.unpack_from("<f", blob, offset)[0]


def vec_sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def vec_cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def vec_normalize(v):
    length = math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])
    if length <= 1.0e-6:
        return (0.0, 1.0, 0.0)
    return (v[0] / length, v[1] / length, v[2] / length)


def parse_tile(tile_data: bytes):
    poly_count = read_i32(tile_data, 24)
    vert_count = read_i32(tile_data, 28)
    max_link_count = read_i32(tile_data, 32)
    detail_mesh_count = read_i32(tile_data, 36)
    detail_vert_count = read_i32(tile_data, 40)
    detail_tri_count = read_i32(tile_data, 44)
    bv_node_count = read_i32(tile_data, 48)
    off_mesh_con_count = read_i32(tile_data, 52)

    offset = align4(MESH_HEADER_SIZE)
    verts_offset = offset
    offset += align4(vert_count * 3 * 4)
    polys_offset = offset
    offset += align4(poly_count * DT_POLY_SIZE)
    offset += align4(max_link_count * DT_LINK_SIZE)
    details_offset = offset
    offset += align4(detail_mesh_count * DT_POLY_DETAIL_SIZE)
    detail_verts_offset = offset
    offset += align4(detail_vert_count * 3 * 4)
    detail_tris_offset = offset
    offset += align4(detail_tri_count * 4)
    offset += align4(bv_node_count * DT_BV_NODE_SIZE)
    offset += align4(off_mesh_con_count * DT_OFF_MESH_CONNECTION_SIZE)

    verts = []
    for i in range(vert_count):
        base = verts_offset + i * 12
        verts.append(
            (
                read_f32(tile_data, base),
                read_f32(tile_data, base + 4),
                read_f32(tile_data, base + 8),
            )
        )

    detail_verts = []
    for i in range(detail_vert_count):
        base = detail_verts_offset + i * 12
        detail_verts.append(
            (
                read_f32(tile_data, base),
                read_f32(tile_data, base + 4),
                read_f32(tile_data, base + 8),
            )
        )

    triangles = []
    for poly_index in range(poly_count):
        poly_base = polys_offset + poly_index * DT_POLY_SIZE
        poly_verts = [
            struct.unpack_from("<H", tile_data, poly_base + 4 + i * 2)[0]
            for i in range(DT_VERTS_PER_POLYGON)
        ]
        vert_count_in_poly = tile_data[poly_base + 30]
        area_and_type = tile_data[poly_base + 31]
        poly_type = area_and_type >> 6
        if poly_type != 0:
            continue

        detail_base = details_offset + poly_index * DT_POLY_DETAIL_SIZE
        detail_vert_base = read_u32(tile_data, detail_base)
        detail_tri_base = read_u32(tile_data, detail_base + 4)
        tri_count = tile_data[detail_base + 9]

        for tri_index in range(tri_count):
            tri_base = detail_tris_offset + (detail_tri_base + tri_index) * 4
            tri = []
            for corner in range(3):
                local_index = tile_data[tri_base + corner]
                if local_index < vert_count_in_poly:
                    tri.append(verts[poly_verts[local_index]])
                else:
                    detail_index = detail_vert_base + (local_index - vert_count_in_poly)
                    tri.append(detail_verts[detail_index])
            triangles.append(tuple(tri))

    return triangles


def load_navmesh_triangles(path: Path):
    blob = path.read_bytes()
    tile_count = read_i32(blob, 8)
    offset = NAVMESH_SET_HEADER_SIZE
    triangles = []

    for _ in range(tile_count):
        if offset + NAVMESH_TILE_HEADER_SIZE > len(blob):
            break
        tile_ref = read_u32(blob, offset)
        data_size = read_i32(blob, offset + 4)
        offset += NAVMESH_TILE_HEADER_SIZE
        if tile_ref == 0 or data_size <= 0:
            break
        tile_data = blob[offset : offset + data_size]
        triangles.extend(parse_tile(tile_data))
        offset += data_size

    return triangles


def pack_vertex(position, normal):
    return struct.pack(
        "<12f",
        position[0],
        position[1],
        position[2],
        normal[0],
        normal[1],
        normal[2],
        1.0,
        0.0,
        0.0,
        1.0,
        0.0,
        0.0,
    )


def build_mesh_payload(triangles):
    vertices = bytearray()
    indices = []
    positions = []

    for tri in triangles:
        normal = vec_normalize(vec_cross(vec_sub(tri[1], tri[0]), vec_sub(tri[2], tri[0])))
        base_index = len(indices)
        for pos in tri:
            vertices.extend(pack_vertex(pos, normal))
            positions.append(pos)
        indices.extend((base_index, base_index + 1, base_index + 2))

    if not positions:
        raise RuntimeError("No navmesh triangles were exported.")

    mins = [min(p[i] for p in positions) for i in range(3)]
    maxs = [max(p[i] for p in positions) for i in range(3)]
    center = [(mins[i] + maxs[i]) * 0.5 for i in range(3)]
    radius = max(
        math.sqrt(sum((p[i] - center[i]) * (p[i] - center[i]) for i in range(3)))
        for p in positions
    )

    index_blob = struct.pack("<II", 32, len(indices))
    index_blob += struct.pack(f"<{len(indices)}I", *indices)

    submesh_blob = struct.pack("<I", 1)
    submesh_blob += struct.pack(
        "<IIIffffff",
        0,
        len(indices),
        0,
        mins[0],
        mins[1],
        mins[2],
        maxs[0],
        maxs[1],
        maxs[2],
    )

    bounds_blob = struct.pack(
        "<ffffffffff",
        mins[0],
        mins[1],
        mins[2],
        maxs[0],
        maxs[1],
        maxs[2],
        center[0],
        center[1],
        center[2],
        radius,
    )

    deps_blob = struct.pack("<I", 0)
    return {
        "VERT": bytes(vertices),
        "INDX": index_blob,
        "SUBM": submesh_blob,
        "BNDS": bounds_blob,
        "DEPS": deps_blob,
    }


def write_evmesh(path: Path, chunks):
    chunk_names = ["VERT", "INDX", "SUBM", "BNDS", "DEPS"]
    table_size = len(chunk_names) * CHUNK_ENTRY_SIZE
    offset = ASSET_HEADER_SIZE + table_size
    entries = bytearray()
    payload = bytearray()

    for name in chunk_names:
        data = chunks[name]
        entries.extend(name.encode("ascii"))
        entries.extend(struct.pack("<IQQQ", 1, offset, len(data), len(data)))
        payload.extend(data)
        offset += len(data)

    file_size = ASSET_HEADER_SIZE + len(entries) + len(payload)
    guid = bytes.fromhex("4E41564D455348444542554700000001")
    header = bytearray()
    header.extend(b"EVMH")
    header.extend(struct.pack("<III", 2, ASSET_HEADER_SIZE, 0))
    header.extend(guid)
    header.extend(struct.pack("<QI", file_size, len(chunk_names)))
    header.extend(bytes(20))

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(bytes(header) + bytes(entries) + bytes(payload))


def main():
    parser = argparse.ArgumentParser(description="Export Detour nav.bin to EisenValor debug evmesh.")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    triangles = load_navmesh_triangles(args.input)
    chunks = build_mesh_payload(triangles)
    write_evmesh(args.output, chunks)
    print(f"Exported {len(triangles)} triangles to {args.output}")


if __name__ == "__main__":
    main()
