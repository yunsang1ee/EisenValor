#include "MeshLoader.h"
#include "AssetFile.h"
#include <cstring>
#include <windows.h>
#include <format>

#ifdef _DEBUG
#define DEBUG_LOG_FMT(fmt, ...) \
    do { \
        std::string _msg = std::format(fmt, __VA_ARGS__); \
        OutputDebugStringA(_msg.c_str()); \
    } while (false)
#else
#define DEBUG_LOG_FMT(fmt, ...) ((void)0)
#endif

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace EvAsset
{
    template <typename T>
    static T ReadUnaligned(const void* ptr)
    {
        T val;
        std::memcpy(&val, ptr, sizeof(T));
        return val;
    }

    bool MeshLoader::LoadMesh(const std::filesystem::path& path, MeshData& outData)
    {
        AssetFile file;
        if (!file.Load(path))
        {
            return false;
        }

        // 헤더 정보 복사
        const auto& header = file.GetHeader();
        outData.assetGuid = header.assetGuid;

        // 1. VERT (Vertex)
        size_t vertSize = 0;
        const void* vertPtr = file.GetChunkDataPtr("VERT", vertSize);
        if (vertPtr)
        {
            size_t count = vertSize / sizeof(Vertex);
            outData.vertices.resize(count);
            std::memcpy(outData.vertices.data(), vertPtr, vertSize);
        }

        // 2. INDX (Index)
        size_t indxSize = 0;
        const uint8_t* indxPtr = static_cast<const uint8_t*>(file.GetChunkDataPtr("INDX", indxSize));
        if (indxPtr && indxSize >= 8)
        {
            uint32_t format = ReadUnaligned<uint32_t>(indxPtr);
            uint32_t count  = ReadUnaligned<uint32_t>(indxPtr + 4);
            const uint8_t* dataStart = indxPtr + 8;
            
            size_t requiredDataSize = count * (format == 16 ? 2 : 4);
            if (indxSize < 8 + requiredDataSize) return false;

            outData.indexFormat = 32; 
            outData.indices.reserve(count);

            if (format == 32)
            {
                outData.indices.resize(count);
                std::memcpy(outData.indices.data(), dataStart, count * 4);
            }
            else if (format == 16)
            {
                for (size_t i = 0; i < count; ++i)
                {
                    uint16_t val;
                    std::memcpy(&val, dataStart + i * 2, 2);
                    outData.indices.push_back(val);
                }
            }
        }

        // 3. SUBM (SubMesh)
        size_t submSize = 0;
        const uint8_t* submPtr = static_cast<const uint8_t*>(file.GetChunkDataPtr("SUBM", submSize));
        if (submPtr && submSize >= 4)
        {
            uint32_t count = ReadUnaligned<uint32_t>(submPtr);
            const uint8_t* dataStart = submPtr + 4;

            if (submSize >= 4 + count * sizeof(SubMesh))
            {
                outData.subMeshes.resize(count);
                std::memcpy(outData.subMeshes.data(), dataStart, count * sizeof(SubMesh));
            }
        }

        // 4. BNDS (Bounds)
        size_t bndsSize = 0;
        const void* bndsPtr = file.GetChunkDataPtr("BNDS", bndsSize);
        if (bndsPtr && bndsSize >= sizeof(Bounds))
        {
            std::memcpy(&outData.boundsInfo, bndsPtr, sizeof(Bounds));
        }

        return outData.IsValid();
    }

    bool MeshLoader::LoadMeshFromObj(const std::filesystem::path& path, MeshData& outData)
    {
        tinyobj::attrib_t                attrib;
        std::vector<tinyobj::shape_t>    shapes;
        std::vector<tinyobj::material_t> materials;
        std::string                      warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str()))
        {
            return false;
        }

        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                Vertex vertex{};

                // Position
                vertex.position[0] = attrib.vertices[3 * index.vertex_index + 0];
                vertex.position[1] = attrib.vertices[3 * index.vertex_index + 1];
                vertex.position[2] = attrib.vertices[3 * index.vertex_index + 2];

                // Normal
                if (index.normal_index >= 0)
                {
                    vertex.normal[0] = attrib.normals[3 * index.normal_index + 0];
                    vertex.normal[1] = attrib.normals[3 * index.normal_index + 1];
                    vertex.normal[2] = attrib.normals[3 * index.normal_index + 2];
                }

                // UV
                if (index.texcoord_index >= 0)
                {
                    vertex.uv0[0] = attrib.texcoords[2 * index.texcoord_index + 0];
                    vertex.uv0[1] = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]; // Flip Y
                }

                // Tangent (Default)
                vertex.tangent[0] = 1.0f;
                vertex.tangent[1] = 0.0f;
                vertex.tangent[2] = 0.0f;
                vertex.tangent[3] = 1.0f;

                outData.vertices.push_back(vertex);
                outData.indices.push_back(static_cast<uint32_t>(outData.indices.size()));
            }
        }

        return outData.IsValid();
    }

    bool MeshLoader::LoadSkinnedMesh(const std::filesystem::path& path, SkinnedMeshData& outData)
    {
        AssetFile file;
        if (!file.Load(path))
        {
            DEBUG_LOG_FMT("[MeshLoader] FAILED to open file: {}\n", path.string());
            return false;
        }

        // 1. 헤더 검증: 시그니처가 EVSK인지 확인
        const auto& header = file.GetHeader();
        if (std::strncmp(header.signature, "EVSK", 4) != 0)
        {
            DEBUG_LOG_FMT("[MeshLoader] WRONG SIGNATURE: Expected EVSK, but got {:.4s}\n", header.signature);
            return false;
        }
        outData.assetGuid = header.assetGuid;

        // 2. VERT 청크 파싱: SkinnedVertex 리스트
        size_t vertSize = 0;
        const void* vertPtr = file.GetChunkDataPtr("VERT", vertSize);
        if (vertPtr)
        {
            size_t count = vertSize / sizeof(SkinnedVertex);
            outData.vertices.resize(count);
            std::memcpy(outData.vertices.data(), vertPtr, vertSize);
            DEBUG_LOG_FMT("[MeshLoader] VERT Loaded: {} vertices\n", count);
        }

        // 3. INDX 청크 파싱: 16/32비트 포맷 대응
        size_t indxSize = 0;
        const uint8_t* indxPtr = static_cast<const uint8_t*>(file.GetChunkDataPtr("INDX", indxSize));
        if (indxPtr && indxSize >= 8)
        {
            uint32_t format = ReadUnaligned<uint32_t>(indxPtr);
            uint32_t count  = ReadUnaligned<uint32_t>(indxPtr + 4);
            const uint8_t* dataStart = indxPtr + 8;
            
            size_t requiredDataSize = count * (format == 16 ? 2 : 4);
            if (indxSize >= 8 + requiredDataSize)
            {
                outData.indexFormat = 32; 
                outData.indices.reserve(count);

                if (format == 32)
                {
                    outData.indices.resize(count);
                    std::memcpy(outData.indices.data(), dataStart, count * 4);
                }
                else if (format == 16)
                {
                    for (size_t i = 0; i < count; ++i)
                    {
                        uint16_t val;
                        std::memcpy(&val, dataStart + i * 2, 2);
                        outData.indices.push_back(val);
                    }
                }
                DEBUG_LOG_FMT("[MeshLoader] INDX Loaded: {} indices\n", count);
            }
        }

        // 4. BONE 청크 파싱: 뼈대(Hierarchy,Rest Pose)
        size_t boneSize = 0;
        const uint8_t* bonePtr = static_cast<const uint8_t*>(file.GetChunkDataPtr("BONE", boneSize));
        if (bonePtr && boneSize >= 4)
        {
            uint32_t count = ReadUnaligned<uint32_t>(bonePtr);
            outData.bones.resize(count);
            std::memcpy(outData.bones.data(), bonePtr + 4, count * sizeof(Bone));
            DEBUG_LOG_FMT("[MeshLoader] BONE Loaded: {} bones\n", count);
        }

        // 5. OFFS 청크 파싱: Inverse Bind Poses
        size_t offSize = 0;
        const void* offPtr = file.GetChunkDataPtr("OFFS", offSize);
        if (offPtr)
        {
            size_t floatCount = offSize / sizeof(float);
            outData.offsetMatrices.resize(floatCount);
            std::memcpy(outData.offsetMatrices.data(), offPtr, offSize);
            DEBUG_LOG_FMT("[MeshLoader] OFFS Loaded: {} matrices\n", floatCount / 16);
        }

        if (!outData.IsValid()) {
            DEBUG_LOG_FMT("[MeshLoader] FINAL IsValid FAILED! (V:{}, I:{}, B:{})\n", 
                outData.vertices.size(), outData.indices.size(), outData.bones.size());
        }

        return outData.IsValid();
    }
}
