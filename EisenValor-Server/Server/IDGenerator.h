#pragma once

namespace Server {
    namespace Contents {
        namespace IDConfig {
            const uint64 TYPE_SHIFT = 60;
            const uint64 WORLD_SHIFT = 44;
            const uint64 SEQ_SHIFT = 0;

            const uint64 TYPE_MASK = 0xF000000000000000;
            const uint64 WORLD_MASK = 0x0FFFF00000000000;
            const uint64 SEQ_MASK = 0x00000FFFFFFFFFFF;
        }

        class IDGenerator {
        public:
            IDGenerator();
       
        public:
            void SetWorldID(uint16 worldID) { m_worldID = worldID; }
            uint64 Generate(uint8 type);

        private:
            uint16 m_worldID;
            uint64 m_seqCounter;
        };
    }
}


