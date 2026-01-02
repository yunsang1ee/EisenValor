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
		requires std::derived_from<SceneType, Scene>
	void RegisterScene(const std::string& sceneName)
	{
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

	void   LoadScene(const std::string& sceneName);
	Scene* GetActiveScene() const { return m_activeScene; }
	Scene* GetScene(const std::string& sceneName) const;

	void SetLocalNetworkID(uint32_t networkID) { m_localNetworkID = networkID; }
	uint32_t GetLocalNetworkID() const { return m_localNetworkID; }

	void					 ClearAllScenes();
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

	uint32_t	m_localNetworkID = 0;
	Scene*		m_activeScene = nullptr;
	std::string m_activeSceneName;
};