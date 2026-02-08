#include "pch.h"
#include "NavSystem.h"

Server::Contents::NavSystem::NavSystem()
	:m_navMesh{ nullptr }, m_navMeshQuery{ nullptr }, m_crowd{nullptr}
{
	m_queryFilter.setIncludeFlags(0xFFFF);
	m_queryFilter.setExcludeFlags(0);
}

Server::Contents::NavSystem::~NavSystem()
{
	if(m_crowd) dtFreeCrowd(m_crowd);
	dtFreeNavMeshQuery(m_navMeshQuery);
	dtFreeNavMesh(m_navMesh);
}

bool Server::Contents::NavSystem::Load(const std::string_view filePath)
{
	std::ifstream ifs{ filePath.data(), std::ios::binary };
	if(!ifs) return false;

	NavMeshSetHeader header;
	ifs.read((char*)&header, sizeof(NavMeshSetHeader));

	const int expectedMagic{ 'T' + ('E' << 8) + ('S' << 16) + ('M' << 24) };

	if(header.magic != expectedMagic) return false;

	m_navMesh = dtAllocNavMesh();
	if(dtStatusFailed(m_navMesh->init(&header.params))) return false;

	for(int i = 0; i < header.numTiles; ++i) {
		NavMeshTileHeader tileHeader;
		ifs.read((char*)&tileHeader, sizeof(NavMeshTileHeader));
		if(!tileHeader.tileRef || !tileHeader.dataSize) break;

		unsigned char* data{ (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM) };
		ifs.read((char*)data, tileHeader.dataSize);
		m_navMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
	}

	m_navMeshQuery = dtAllocNavMeshQuery();
	if(dtStatusFailed(m_navMeshQuery->init(m_navMesh, 2048))) return false;

	m_crowd = dtAllocCrowd();
	
	constexpr int32 maxAgents{ 10 };
	constexpr float maxAgentRadius{ 2.f };

	if(!m_crowd->init(maxAgents, maxAgentRadius, m_navMesh)) return false;

	dtObstacleAvoidanceParams params;
	memcpy(&params, m_crowd->getObstacleAvoidanceParams(0), sizeof(dtObstacleAvoidanceParams));

	params.velBias = 0.5f;
	params.adaptiveDivs = 5;
	params.adaptiveRings = 2;
	params.adaptiveDepth = 2;
	m_crowd->setObstacleAvoidanceParams(0, &params);

	return true;
}

void Server::Contents::NavSystem::Update(const float dt)
{
	if(m_crowd)
		m_crowd->update(dt, nullptr);
}

void Server::Contents::NavSystem::SetMoveTarget(const int32 agentIdx, const Vec3& targetPos)
{
	if(!m_crowd) return;

	const dtCrowdAgent* ag{ m_crowd->getAgent(agentIdx) };
	if(!ag || !ag->active) return;

	float pos[3] = { targetPos.x, targetPos.y, targetPos.z };
	constexpr float searchRange[3] = { 2.0f, 10.0f, 2.0f }; // 검색 범위
	dtPolyRef targetRef;
	float targetNearest[3];

	m_navMeshQuery->findNearestPoly(pos, searchRange, &m_queryFilter, &targetRef, targetNearest);

	if(targetRef) {
		m_crowd->requestMoveTarget(agentIdx, targetRef, targetNearest);
	}
}