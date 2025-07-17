#pragma once


//template<typename Type, uint32 Size>
//using EV_ARRAY = std::array<Type, Size>;
//
//template<typename Type>
//using EV_VECTOR = std::vector<Type, tbb::memory_pool_allocator<Type>>;
//template<typename Type>
//EV_VECTOR<Type> MAKE_EV_VECTOR() { return EV_VECTOR(tbb::memory_pool_allocator<Type>(ServerEngine::MemoryPool::m_pool)); }
//template<typename Type>
//EV_VECTOR<Type> MAKE_EV_VECTOR(const size_t size) { return EV_VECTOR<Type>(size, tbb::memory_pool_allocator<Type>(ServerEngine::MemoryPool::m_pool)); }
//template<typename Type>
//EV_VECTOR<Type> MAKE_EV_VECTOR(const size_t size, const Type& value) { return EV_VECTOR<Type>(size, value, tbb::memory_pool_allocator<Type>(ServerEngine::MemoryPool::m_pool)); }
//
//template<typename Key, typename Type, typename Pred = std::less<Key>>
//using EV_MAP = std::map<Key, Type, Pred, tbb::memory_pool_allocator<std::pair<const Key, Type>>>;
//
//template<typename Key, typename Type, typename Hasher = std::hash<Key>, typename KeyEq = std::equal_to<Key>>
//using EV_HASH_MAP = std::unordered_map<Key, Type, Hasher, KeyEq, tbb::memory_pool_allocator<std::pair<const Key, Type>>>;
//
//template<typename Key, typename Pred = std::less<Key>>
//using EV_SET = std::set<Key, Pred, tbb::memory_pool_allocator<Key>>;
//
//template<typename Key, typename Hasher = std::hash<Key>, typename KeyEq = std::equal_to<Key>>
//using EV_HASH_SET = std::unordered_set<Key, Hasher, KeyEq, tbb::memory_pool_allocator<Key>>;
