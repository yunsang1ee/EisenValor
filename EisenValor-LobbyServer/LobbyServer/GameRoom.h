#pragma once

namespace LobbyServerEngine {
	class PacketBuffer;
}

namespace LobbyServer {
	class ClientSession;
	class Participant;
	class User;
	class Bot;
	
	using Users = std::unordered_map<uint32, std::shared_ptr<User>>;
	using Bots = std::unordered_map<uint32, std::shared_ptr<Bot>>;

	class GameRoom : public std::enable_shared_from_this<GameRoom> {
	public:
		GameRoom();
		~GameRoom();
		
	public:
		void Broadcast(std::shared_ptr<LobbyServerEngine::PacketBuffer> pb);
		void EnterGameRoom(const std::shared_ptr<ClientSession>& clientSession);
		void LeaveGameRoom(const std::shared_ptr<ClientSession>& clientSession);

	public:
		void ChangeTeam(const std::shared_ptr<ClientSession>& clientSession);
		void AddBot(const std::shared_ptr <ClientSession>& clientSession, const FB_ENUMS::TEAM_TYPE botTeamType);
		void RemoveBot(const std::shared_ptr<ClientSession>& clientSession, const uint32 botID);
		void ReadyGame(const std::shared_ptr<ClientSession>& clientSession);
		void StartGame(const std::shared_ptr<ClientSession>& clientSession);
		void ReturnToGameRoom(const std::shared_ptr<ClientSession>& clientSession);
		void ApplyGameResult(const FB_ENUMS::TEAM_TYPE winningTeam, const uint8 blueScore, const uint8 redScore);

		std::shared_ptr<User> GetSessionUser(const std::shared_ptr<ClientSession>& clientSession);

	public:
		uint32 GetNewBotID() { return ++m_idGenerator; }

	public:
		void SetRoomInfo(const RoomInfo& info) { m_info = info; }
		void SetRoomState(const FB_ENUMS::ROOM_STATE_TYPE stateType) { m_info.stateType = stateType; }
		const RoomInfo& GetRoomInfo() const { return m_info; }

	private:
		void EnterParticipant(std::shared_ptr<Participant> participant);

	private:
		RoomInfo													m_info;
		uint32														m_idGenerator;

		Users														m_users;
		Bots														m_bots;
		std::shared_ptr<User>										m_host;

		int32														m_blueTeamCount;
		int32														m_redTeamCount;
		bool														m_gameResultApplied;
	};
}

