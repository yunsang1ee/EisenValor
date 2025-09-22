#pragma once
#include <tbb/tbb.h>
#include <tbb/memory_pool.h>
#include <tbb/scalable_allocator.h>

namespace ServerEngine {
    template<typename Type>
    class ObjectPool {
    private:
        using Pool = oneapi::tbb::memory_pool<oneapi::tbb::scalable_allocator<char>>;
        static Pool m_pool;

    public:
        ObjectPool()
        {
        }

        ~ObjectPool()
        {
            m_pool.recycle();
        }

        template<typename... Args>
        static Type* Pop(Args&&... args)
        {
            Type* obj = static_cast<Type*>(m_pool.malloc(sizeof(Type)));
            new(obj)Type(std::forward<Args>(args)...);
            return obj;
        }

        static void Push(Type* const obj)
        {
            if(obj) {
                obj->~Type();
                m_pool.free(obj);
            }
        }

        template<typename... Args>
        static std::shared_ptr<Type> MakeShared(Args&&... args)
        {
            std::shared_ptr<Type> ptr{ Pop(std::forward<Args>(args)...), Push };
            return ptr;
        }
    };

    template<typename Type>
    tbb::memory_pool<oneapi::tbb::scalable_allocator<char>> ObjectPool<Type>::m_pool;
}