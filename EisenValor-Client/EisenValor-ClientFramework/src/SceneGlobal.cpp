#include "stdafxClientFramework.h"
#include "SceneGlobal.h"

void SceneGlobal::Initialize()
{
	DEBUG_LOG_FMT("[SceneGlobal] Initialized\n");
}

void SceneGlobal::Release()
{
	ClearAllScenes();
	DEBUG_LOG_FMT("[SceneGlobal] Released\n");
}

void SceneGlobal::LoadScene(const std::string& sceneName)
{
	if (m_activeScene && m_activeSceneName == sceneName)
	{
		DEBUG_LOG_FMT("[SceneGlobal] Scene already active: {}\n", sceneName);
		return;
	}

	auto iter = m_scenes.find(sceneName);
	if (iter == m_scenes.end())
	{
		DEBUG_LOG_FMT("[SceneGlobal] Scene not found: {}\n", sceneName);
		assert(false && "Scene not registered!");
		return;
	}

	if (m_activeScene)
	{
		DEBUG_LOG_FMT("[SceneGlobal] Unloading previous scene: {}\n", m_activeSceneName);
		m_activeScene->OnEnd();
	}

	m_activeScene = iter->second.get();
	m_activeScene->SetLocalID(m_localNetworkID);
	m_activeSceneName = sceneName;

	m_activeScene->OnStart();

	DEBUG_LOG_FMT("[SceneGlobal] Scene loaded: {}\n", sceneName);
}

Scene* SceneGlobal::GetScene(const std::string& sceneName) const
{
	auto iter = m_scenes.find(sceneName);
	if (iter != m_scenes.end())
	{
		return iter->second.get();
	}
	return nullptr;
}

void SceneGlobal::ClearAllScenes()
{
	for (auto& [name, scene] : m_scenes)
	{
		if (scene)
		{
			DEBUG_LOG_FMT("[SceneGlobal] Clearing scene: {}\n", name);
			scene->OnEnd();
		}
	}
	m_activeScene = nullptr;
	m_activeSceneName.clear();
	m_scenes.clear();
	DEBUG_LOG_FMT("[SceneGlobal] All scenes cleared\n");
}

std::vector<std::string> SceneGlobal::GetSceneNames() const
{
	std::vector<std::string> names;
	names.reserve(m_scenes.size());
	for (const auto& [name, _] : m_scenes)
	{
		names.push_back(name);
	}
	return names;
}


void SceneGlobal::OnBeginFrame()
{
	if (m_activeScene)
	{
		m_activeScene->OnBeginFrame();
	}
}

void SceneGlobal::OnUpdate(float deltaTime)
{
	if (m_activeScene)
	{
		m_activeScene->OnUpdate(deltaTime);
	}
}

void SceneGlobal::OnFixedUpdate(float fixedDeltaTime)
{
	if (m_activeScene)
	{
		m_activeScene->OnFixedUpdate(fixedDeltaTime);
	}
}

void SceneGlobal::OnLateUpdate(float deltaTime)
{
	if (m_activeScene)
	{
		m_activeScene->OnLateUpdate(deltaTime);
	}
}

void SceneGlobal::OnEndFrame()
{
	if (m_activeScene)
	{
		m_activeScene->OnEndFrame();
	}
}