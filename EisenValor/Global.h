#pragma once
#include "ISystem.h"

template <typename T>
concept NoCopyMove = !(std::is_copy_constructible_v<T>
    || std::is_move_constructible_v<T>);

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
    template<typename Interface>
        requires IsGlobal<Interface> && NoCopyMove<Interface>
    Interface& Register(std::unique_ptr<Interface> instance) {
        assert(instance != nullptr);
        return *(registry<Interface>() = std::move(instance));
    }

    template<IsGlobal Interface>
    Interface& Get() {
        auto& ptr = registry<Interface>();
        assert(ptr != nullptr && "Global: Not Registered");
        return *ptr;
    }

    template <IsGlobal Interface>
    void Reset() {
        registry<Interface>().reset();
    }

private:
    template<IsGlobal Interface>
    std::unique_ptr<Interface>& registry() {
        static std::unique_ptr<Interface> instance;
        return instance;
    }
};