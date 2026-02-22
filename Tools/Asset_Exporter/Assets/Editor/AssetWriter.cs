using System.Collections.Generic;
using System.IO;
using System.Text;

// ========================================================
// Asset Writer Helper (Generic Builder)
// ========================================================
public class AssetWriter
{
    private string _signature;
    private string _guidHex;
    private List<ChunkEntry> _chunks = new List<ChunkEntry>();

    private struct ChunkEntry
    {
        public string Type;
        public uint Version;
        public byte[] Data;
    }

    public AssetWriter(string signature, string guidHex)
    {
        if (signature.Length != 4) throw new System.ArgumentException("Signature must be 4 chars");
        _signature = signature;
        _guidHex = guidHex;
    }

    public void AddChunk(string type, uint version, byte[] data)
    {
        if (type.Length != 4) throw new System.ArgumentException("Chunk Type must be 4 chars");
        _chunks.Add(new ChunkEntry { Type = type, Version = version, Data = data });
    }

    public void WriteToFile(string path)
    {
        using (FileStream fs = new FileStream(path, FileMode.Create))
        using (BinaryWriter bw = new BinaryWriter(fs))
        {
            uint headerSize = 64;
            uint chunkTableSize = (uint)(_chunks.Count * 32); // 32 bytes per ChunkEntry (Type:4, Ver:4, Off:8, Size:8, UnSize:8)
            ulong currentPayloadOffset = headerSize + chunkTableSize;

            bw.Write(Encoding.ASCII.GetBytes(_signature)); // Signature
            bw.Write((uint)2); // Version (v2.1)
            bw.Write(headerSize);
            bw.Write((uint)0); // Flags
            
            byte[] guidBytes = HexToBytes(_guidHex);
            bw.Write(guidBytes); // AssetGuid (16 bytes)

            long fileSizePos = bw.BaseStream.Position;
            bw.Write((ulong)0); // FileSize placeholder

            bw.Write((uint)_chunks.Count);
            bw.Write(new byte[20]); // Reserved (padding to 64 bytes)

            foreach (var chunk in _chunks)
            {
                bw.Write(Encoding.ASCII.GetBytes(chunk.Type));
                bw.Write(chunk.Version);
                bw.Write(currentPayloadOffset);
                bw.Write((ulong)chunk.Data.Length); // Size
                bw.Write((ulong)chunk.Data.Length); // UncompressedSize (same for now)

                currentPayloadOffset += (ulong)chunk.Data.Length;
            }

            foreach (var chunk in _chunks)
            {
                bw.Write(chunk.Data);
            }

            ulong totalSize = (ulong)bw.BaseStream.Position;
            bw.BaseStream.Seek(fileSizePos, SeekOrigin.Begin);
            bw.Write(totalSize);
        }
    }

    private static byte[] HexToBytes(string hex)
    {
        if (string.IsNullOrEmpty(hex) || hex.Length != 32) return new byte[16];
        byte[] bytes = new byte[16];
        for (int i = 0; i < 16; i++)
            bytes[i] = System.Convert.ToByte(hex.Substring(i * 2, 2), 16);
        return bytes;
    }
}
