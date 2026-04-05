#pragma once
#include "GameObject.h"
#include <cstddef>
#include <cstdint>
#include <concepts>
#include <vector>

struct SceneComponentLoadContext
{
	GameObject::Handle ownerHandle = GameObject::Handle::Invalid();
	uint32_t		   nodeIndex = 0;
};

template <typename T>
concept SceneComponentPayload = requires(T payload, uint32_t version, const std::vector<std::byte>& data) {
	{ T::StaticTypeId() } -> std::convertible_to<uint64_t>;
	{ payload.Deserialize(version, data) } -> std::same_as<bool>;
};
