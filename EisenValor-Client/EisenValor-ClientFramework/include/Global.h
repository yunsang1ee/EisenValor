#pragma once
#include "IGlobal.h"
#include <unordered_map>

template <typename T>
concept IsRegisterable = IsGlobal<T> 
    && !std::is_copy_constructible_v<T>
    && !std::is_move_constructible_v<T>;

//class GlobalRegistry {
//public:
//    template<IsGlobal Interface>
//    Interface& Register(std::unique_ptr<Interface> instance) {
//        assert(instance != nullptr);
//		return registry_[std::type_index{ typeid(Interface) }] = std::move(instance);
//    }
//
//    template<IsGlobal Interface>
//    Interface& Get() {
//        auto& ptr = registry_[std::type_index{ typeid(Interface) }];
//        assert(ptr != nullptr && "Global: Not Registered");
//        return *ptr;
//    }
//
//    template <IsGlobal Interface>
//    void Reset() {
//        registry_[std::type_index{ typeid(Interface) }].reset();
//    }
//
//private:
//    template<IsGlobal Interface>
//    Interface& registry() {
//        static 
//        assert(instance != nullptr);
//        return registry_[std::type_index{ typeid(Interface) }] = std::move(instance);
//    }
//
//    std::unordered_map<std::type_index, std::unique_ptr<IGlobal>> registry_;
//};


class GlobalRegistry {
public:
    enum class RegistryType
    {
        Main,
        Mock,
        Test,
    };

    template<IsRegisterable Interface>
    static Interface& Register(RegistryType id, std::unique_ptr<Interface> instance) {
        assert(instance != nullptr && "Global: is null instance.");
        active<Interface>() = id;
        return *(registryMap<Interface>()[id] = std::move(instance));
    }

    template<IsRegisterable Interface>
    static void SetActive(RegistryType id) {
        assert(registryMap<Interface>().count(id) && "Global: not Found id");
        active<Interface>() = id;
    }

    template<IsRegisterable Interface>
    static Interface& Get() {
        auto& ptr = registryMap<Interface>()[active<Interface>()];
        assert(ptr != nullptr && "Global: Not Registered");
        return *ptr;
    }

    template<IsRegisterable Interface>
    static void Reset(RegistryType id) {
        registryMap<Interface>().erase(id);
        if (active<Interface>() == id)
            active<Interface>() = {};
    }

private:
    template<IsRegisterable Interface>
    static std::unordered_map<RegistryType, std::unique_ptr<Interface>>& registryMap() {
        static std::unordered_map<RegistryType, std::unique_ptr<Interface>> map;
        return map;
    }

    template<IsRegisterable Interface>
    static RegistryType& active() {
        static RegistryType current{};
        return current;
    }
};