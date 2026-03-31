#pragma once

namespace LobbyServerEngine {
    class ConfigManager {
    public:
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;
        ConfigManager(ConfigManager&&) = delete;
        ConfigManager& operator=(ConfigManager&&) = delete;

        static ConfigManager& GetInstance()
        {
            static ConfigManager instance;
            return instance;
        }
 
        void LoadFile(std::string_view filePath);

        template<typename... Keys>
        const char* GetString(Keys&&... keys) const
        {
            const rapidjson::Value& val = Resolve(std::forward<Keys>(keys)...);
            AssertType(val, "string");
            return val.GetString();
        }

        template<typename... Keys>
        int GetInt(Keys&&... keys) const
        {
            const rapidjson::Value& val = Resolve(std::forward<Keys>(keys)...);
            AssertType(val, "int");
            return val.GetInt();
        }

        template<typename... Keys>
        uint16_t GetUInt16(Keys&&... keys) const
        {
            const rapidjson::Value& val = Resolve(std::forward<Keys>(keys)...);
            AssertType(val, "uint");
            const unsigned int raw = val.GetUint();
            if(raw > std::numeric_limits<uint16_t>::max()) {
                throw std::runtime_error(
                    "ConfigManager: Value " + std::to_string(raw) +
                    " exceeds uint16_t range (0~65535)"
                );
            }
            return static_cast<uint16_t>(raw);
        }

        template<typename... Keys>
        uint32_t GetUInt32(Keys&&... keys) const
        {
            const rapidjson::Value& val = Resolve(std::forward<Keys>(keys)...);
            AssertType(val, "uint");
            return val.GetUint();
        }

        template<typename... Keys>
        double GetDouble(Keys&&... keys) const
        {
            const rapidjson::Value& val = Resolve(std::forward<Keys>(keys)...);
            AssertType(val, "double");
            return val.GetDouble();
        }

        template<typename... Keys>
        bool GetBool(Keys&&... keys) const
        {
            const rapidjson::Value& val = Resolve(std::forward<Keys>(keys)...);
            AssertType(val, "bool");
            return val.GetBool();
        }

        uint32_t GetWorkerThreadCount() const
        {
            const unsigned int val = GetUInt32("LobbyServer", "WorkerThread");
            if(val == 0) {
                const unsigned int hw = std::thread::hardware_concurrency();
                return (hw > 0) ? hw : 1u; // hardware_concurrency가 0 반환 시 최소 1
            }
            return val;
        }

        uint32_t GetMaxSession() const
        {
            return GetUInt32("LobbyServer", "MaxSession");
        }

        std::string_view GetFilePath() const noexcept { return _filePath; }

        const rapidjson::Document& GetDocument() const noexcept { return _document; }

    private:
        ConfigManager() = default;
        ~ConfigManager() = default;

        // ── 내부 헬퍼 ────────────────────────────────────────────────────────────

        // 단일 키 resolve
        const rapidjson::Value& ResolveOne(const rapidjson::Value& node, const char* key) const
        {
            if(!node.IsObject())
                throw std::runtime_error(std::string("ConfigManager: Expected object for key: ") + key);

            auto it = node.FindMember(key);
            if(it == node.MemberEnd())
                throw std::runtime_error(std::string("ConfigManager: Key not found: ") + key);

            return it->value;
        }
 
        template<typename... Keys>
        const rapidjson::Value& Resolve(const char* first, Keys&&... rest) const
        {
            const rapidjson::Value& node = ResolveOne(_document, first);
            if constexpr(sizeof...(rest) == 0)
                return node;
            else
                return ResolveChain(node, std::forward<Keys>(rest)...);
        }

        template<typename... Keys>
        const rapidjson::Value& ResolveChain(const rapidjson::Value& node, const char* first, Keys&&... rest) const
        {
            const rapidjson::Value& next = ResolveOne(node, first);
            if constexpr(sizeof...(rest) == 0)
                return next;
            else
                return ResolveChain(next, std::forward<Keys>(rest)...);
        }

        void AssertType(const rapidjson::Value& val, std::string_view expected) const;

        rapidjson::Document _document;
        std::string         _filePath;
    };
}