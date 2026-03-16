#pragma once

namespace Server {
	namespace Contents {
		struct NavMeshSetHeader {
			int32 magic;
			int32 version;
			int32 numTiles;
			dtNavMeshParams params;
		};

		struct NavMeshTileHeader {
			dtTileRef tileRef;
			int32 dataSize;
		};

		class NavSystem {
		public:
			NavSystem();
			~NavSystem();

		public:
			bool Load(const std::string_view filePath);
			void Update(const float dt);
			
			void SetMoveTarget(const int32 agentIdx, const Vec3& targetPos);
			void ResetMoveTarget(const int32 agentIdx);
			void RemoveAgent(const int32 agentIdx);

			dtCrowd* GetCrowd() const { return m_crowd; }

		private:
			dtNavMesh*		m_navMesh;

			dtNavMeshQuery* m_navMeshQuery;
			dtQueryFilter	m_queryFilter;

			dtCrowd*		m_crowd;
		};
	}
}

