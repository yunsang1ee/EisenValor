#pragma once

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

#define NOMINMAX

#define SINGLETON_CLASS(classname)                      \
private:                                                \
    classname() = default;                              \
    ~classname() = default;                             \
    classname(const classname&) = delete;               \
    classname& operator=(const classname&) = delete;    \
    classname(classname&&) = delete;                    \
    classname& operator=(classname&&) = delete;         \
                                                        \
public:                                                 \
    static classname* GetInstance()                     \
    {                                                   \
        static classname instance;                      \
        return &instance;                               \
    }

#define MANAGER(classname) (classname::GetInstance())