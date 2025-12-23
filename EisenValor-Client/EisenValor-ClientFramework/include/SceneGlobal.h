#pragma once
#include "Singleton.h"
#include "Scene.h"

class SceneGlobal : public Singleton<SceneGlobal>
{
private:
	friend class Singleton<SceneGlobal>;

	SceneGlobal() = default;
	~SceneGlobal() override = default;

public:
	void Initialize() override;
	void Release() override;

	template <typename SceneType>
	void RegisterScene(const std::string& sceneName)
	{
		static_assert(std::derived_from<SceneType, Scene>, "SceneType must derive from Scene");

		if (m_scenes.contains(sceneName))
		{
			DEBUG_LOG_FMT("[SceneGlobal] Scene already registered: {}\n", sceneName);
			return;
		}

		auto scene = std::make_unique<SceneType>();
		scene->Initialize();
		m_scenes[sceneName] = std::move(scene);

		DEBUG_LOG_FMT("[SceneGlobal] Scene registered: {}\n", sceneName);
	}

	void					 LoadScene(const std::string& sceneName);
	void					 UnloadScene(const std::string& sceneName);
	Scene*					 GetActiveScene() const { return m_activeScene; }
	Scene*					 GetScene(const std::string& sceneName) const;

	void					 ClearAllScenes();
	void					 RemoveScene(const std::string& sceneName);
	std::vector<std::string> GetSceneNames() const;

	//========================================================================
	// Frame Lifecycle
	//========================================================================

	void OnBeginFrame();
	void OnUpdate(float deltaTime);
	void OnFixedUpdate(float fixedDeltaTime);
	void OnLateUpdate(float deltaTime);
	void OnEndFrame();

private:
	std::unordered_map<std::string, std::unique_ptr<Scene>> m_scenes;

	Scene*		m_activeScene = nullptr;
	std::string m_activeSceneName;
};