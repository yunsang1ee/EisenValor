#include "pch.h"
#include "IDGenerator.h"

GameServer::Contents::IDGenerator::IDGenerator()
    : m_worldID{}, m_seqCounter{ 1 }
{
}

uint64 GameServer::Contents::IDGenerator::Generate(uint8 type)
{
    uint64 currentSeq = m_seqCounter++;

    uint64 id = 0;
    id |= (static_cast<uint64>(type) << IDConfig::TYPE_SHIFT) & IDConfig::TYPE_MASK;
    id |= (static_cast<uint64>(m_worldID) << IDConfig::WORLD_SHIFT) & IDConfig::WORLD_MASK;
    id |= (currentSeq << IDConfig::SEQ_SHIFT) & IDConfig::SEQ_MASK;

    return id;
}
