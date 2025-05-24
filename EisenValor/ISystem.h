#pragma once
#include <concepts>

class IGlobal {
public:
    virtual ~IGlobal() = default;
};

template<typename Derive>
concept IsGlobal = std::derived_from<Derive, IGlobal>;

template<typename Derived, IsGlobal Interface>
class GlobalMakerBase : public Interface {
public:// Local Maker idiom
    template<typename... Args>
    static std::unique_ptr<Derived> Create(Args&&... args) {
        struct Maker : Derived {
            Maker(Args&&... a) : Derived(std::forward<Args>(a)...) {}
        };
        return std::make_unique<Maker>(std::forward<Args>(args)...);
    }

protected:
    GlobalMakerBase() = default;
    ~GlobalMakerBase() = default;

    GlobalMakerBase(const GlobalMakerBase&) = delete;
    GlobalMakerBase(GlobalMakerBase&&) = delete;
    GlobalMakerBase& operator=(const GlobalMakerBase&) = delete;
    GlobalMakerBase& operator=(GlobalMakerBase&&) = delete;
};