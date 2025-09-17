#pragma once

#include "../../EisenValor-Server/ServerEngine/Singleton.hpp"

class GameObject;
class LocalPlayer;

class GameObjectManager : public Singleton<GameObjectManager>
{
public:
	GameObjectManager() = default;
	virtual ~GameObjectManager() = default;
	friend class Singleton;

private:
	std::vector<std::shared_ptr<GameObject>> m_gameObjects;
	std::shared_ptr<LocalPlayer>			 m_localPlayer;
	uint32									 m_localID;

public:
	void Update(const float dt);
	void Render(
		ID3D12GraphicsCommandList* const cmdList, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection
	);

	void FinalUpdate();

public:
	void AddObject(std::shared_ptr<GameObject> object);
	void RemoveObject(const uint32 id);

public:
	void SetLocalID(const uint32 id) { m_localID = id; }
	void SetLocalPlayer(std::shared_ptr<LocalPlayer> localPlayer) { m_localPlayer = localPlayer; }
	std::shared_ptr<LocalPlayer> GetLocalPlayer() { return m_localPlayer; }
	uint32						 GetLocalID() { return m_localID; }
	std::shared_ptr<GameObject>	 FindObject(const uint32 id);
	std::vector<std::shared_ptr<GameObject>>& GetAllObjects() { return m_gameObjects; }
};
