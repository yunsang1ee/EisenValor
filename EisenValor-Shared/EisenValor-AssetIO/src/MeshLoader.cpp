#include "MeshLoader.h"
#include "AssetFile.h"
#include <cstring>

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
}
