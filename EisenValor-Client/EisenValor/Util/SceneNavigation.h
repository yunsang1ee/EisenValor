#pragma once

#include "SceneGlobal.h"
#include <string>
#include <utility>

namespace SceneNavigation
{
inline std::string pendingScene;

// UI callbacks must finish before their scene's component storage is destroyed.
inline void Request(std::string sceneName)
{
	pendingScene = std::move(sceneName);
}

inline void ApplyPending()
{
	if (!pendingScene.empty())
	{
		const auto sceneName = std::exchange(pendingScene, {});
		GLOBAL(SceneGlobal).LoadScene(sceneName);
	}
}
} // namespace SceneNavigation
