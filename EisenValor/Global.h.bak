#pragma once
#include "IGlobal.h"
#include <unordered_map>
#include <string_view>

template <typename T>
concept IsRegistrable = IsGlobal<T>
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
    using SlotID = std::string_view;

    template<IsRegistrable Interface>
    static Interface& Register(SlotID id, std::unique_ptr<Interface> instance) {
        assert(instance != nullptr && "Global: is null instance.");
        SetActive<Interface>(id);
        return *(registryMap<Interface>()[id] = std::move(instance));
    }

    template<IsRegistrable Interface>
    static void SetActive(SlotID id) {
        assert(registryMap<Interface>().count(id));
        active<Interface>() = id;
    }

    template<IsRegistrable Interface>
    static Interface& Get() {
        auto& ptr = registryMap<Interface>()[active<Interface>()];
        assert(ptr != nullptr && "Global: Not Registered");
        return *ptr;
    }

    template<IsRegistrable Interface>
    static void Reset(SlotID id) {
        registryMap<Interface>().erase(id);
        if (active<Interface>() == id)
            active<Interface>() = {};
    }

private:
    template<IsRegistrable Interface>
    static std::unordered_map<SlotID, std::unique_ptr<Interface>>& registryMap() {
        static std::unordered_map<SlotID, std::unique_ptr<Interface>> map;
        return map;
    }

    template<IsRegistrable Interface>
    static SlotID& active() {
        static SlotID current{};
        return current;
    }
};