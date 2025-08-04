#include "stdafxClientFramework.h"
#include "GameObjectManager.h"

#include "GameObject.h"

void GameObjectManager::Update(const float dt)
{
	for(const auto& obj : m_gameObjects)
		if(obj && obj->alive) obj->Update(dt);
}

void GameObjectManager::Render(ID3D12GraphicsCommandList* const cmdList, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection)
{
	for(const auto& obj : m_gameObjects)
		if(obj && obj->alive) obj->Render(cmdList, view, projection);
}

void GameObjectManager::FinalUpdate()
{
	//auto iter = m_gameObjects.begin();

	//for(; iter != m_gameObjects.end();) {
	//	if((*iter)->alive == false) {
	//		m_gameObjects.erase(iter);
	//		iter = iter;
	//	}
	//}
}

void GameObjectManager::AddObject(std::shared_ptr<GameObject> object)
{
	m_gameObjects.push_back(object);
}

void GameObjectManager::RemoveObject(const uint32 id)
{
	auto iter = std::find_if(m_gameObjects.begin(), m_gameObjects.end(), [id](const auto& obj) { return obj->m_id == id; });

	if(iter != m_gameObjects.end())
		m_gameObjects.erase(iter);
}

std::shared_ptr<GameObject> GameObjectManager::FindObject(const uint32 id)
{
	auto iter = std::find_if(m_gameObjects.begin(), m_gameObjects.end(), [id](const auto& obj) { return obj->m_id == id; });
	if(iter != m_gameObjects.end())
		return (*iter);

	return nullptr;
}
