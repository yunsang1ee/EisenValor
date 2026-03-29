#pragma once
#include <tbb/tbb.h>
#include <tbb/memory_pool.h>
#include <tbb/scalable_allocator.h>

namespace GameServerEngine {
    template<typename Type>
    class ObjectPool {
    public:
        ObjectPool()
        {
            std::cout << "ObjectPool" << std::endl;
        }

        ~ObjectPool()
        {
            std::cout << "~ObjectPool" << std::endl;
        }

        template<typename... Args>
        static Type* Pop(Args&&... args)
        {
            Type* obj = static_cast<Type*>(m_pool.malloc(sizeof(Type)));
            return std::construct_at(obj, std::forward<Args>(args)...);
        }

        static void Push(Type* const obj)
        {
            if(obj) {
                std::destroy_at(obj);
                m_pool.free(obj);
            }
        }

        template<typename... Args>
        static std::shared_ptr<Type> MakeShared(Args&&... args)
        {
            std::shared_ptr<Type> ptr{ Pop(std::forward<Args>(args)...), Push };
            return ptr;
        }

    private:
        using Pool = oneapi::tbb::memory_pool<oneapi::tbb::scalable_allocator<Type>>;
        static Pool m_pool;
    };

    template<typename Type>
    tbb::memory_pool<oneapi::tbb::scalable_allocator<Type>> ObjectPool<Type>::m_pool;
}