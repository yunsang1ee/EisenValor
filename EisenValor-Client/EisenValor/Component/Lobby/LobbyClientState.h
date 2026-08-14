#pragma once

#include "Packets/Enums_generated.h"
#include "Singleton.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

struct LobbyRoomView
{
	uint16					  id = 0;
	FB_ENUMS::ROOM_STATE_TYPE state = FB_ENUMS::ROOM_STATE_TYPE_WATING;
	uint8					  currentParticipants = 0;
	uint8					  maxParticipants = 0;
};

struct LobbyUserView
{
	uint32		id = 0;
	std::string name;
};

struct RoomParticipantView
{
	uint32							 id = 0;
	FB_ENUMS::PARTICIPANT_TYPE		 type = FB_ENUMS::PARTICIPANT_TYPE_USER;
	FB_ENUMS::PARTICIPANT_STATE_TYPE state = FB_ENUMS::PARTICIPANT_STATE_TYPE_NOT_READY;
	FB_ENUMS::TEAM_TYPE				 team = FB_ENUMS::TEAM_TYPE_NONE;
};

class LobbyClientState : public Singleton<LobbyClientState>
{
private:
	friend class Singleton<LobbyClientState>;

	LobbyClientState() = default;
	~LobbyClientState() override = default;

public:
	static constexpr size_t kRoomsPerPage = 6;

	void ResetLobby()
	{
		m_rooms.clear();
		m_roomIndexByID.clear();
		m_usersByID.clear();
		m_message.clear();
		m_lobbyPage = 0;
		Touch();
	}

	void ReserveLobby(size_t roomCount, size_t userCount)
	{
		m_rooms.reserve(roomCount);
		m_roomIndexByID.reserve(roomCount);
		m_usersByID.reserve(userCount);
	}

	void PreviousLobbyPage()
	{
		const size_t currentPage = GetLobbyPage();
		if (currentPage == 0)
		{
			return;
		}
		m_lobbyPage = currentPage - 1;
		Touch();
	}

	void NextLobbyPage()
	{
		const size_t currentPage = GetLobbyPage();
		if (currentPage + 1 >= GetLobbyPageCount())
		{
			return;
		}
		m_lobbyPage = currentPage + 1;
		Touch();
	}

	void AddOrUpdateRoom(const LobbyRoomView& room)
	{
		const auto iter = m_roomIndexByID.find(room.id);
		if (iter != m_roomIndexByID.end())
		{
			m_rooms[iter->second] = room;
			Touch();
			return;
		}

		m_roomIndexByID.emplace(room.id, m_rooms.size());
		m_rooms.push_back(room);
		Touch();
	}

	void RemoveRoom(uint16 id)
	{
		const auto iter = m_roomIndexByID.find(id);
		if (iter == m_roomIndexByID.end())
		{
			return;
		}

		const size_t removedIndex = iter->second;
		m_rooms.erase(m_rooms.begin() + static_cast<ptrdiff_t>(removedIndex));
		m_roomIndexByID.erase(iter);

		// 뒤쪽 방들의 인덱스가 하나씩 당겨졌으므로 같이 보정한다.
		for (auto& [roomID, index] : m_roomIndexByID)
		{
			if (index > removedIndex)
			{
				--index;
			}
		}
		Touch();
	}

	void AddOrUpdateUser(uint32 id, std::string name)
	{
		m_usersByID.insert_or_assign(id, LobbyUserView{id, std::move(name)});
		Touch();
	}

	void RemoveUser(uint32 id)
	{
		if (m_usersByID.erase(id) > 0)
		{
			Touch();
		}
	}

	void ResetRoom(const RoomParticipantView& localParticipant)
	{
		m_participants.clear();
		m_localParticipantID = localParticipant.id;
		m_startPending = false;
		m_message.clear();
		AddOrUpdateParticipant(localParticipant);
	}

	void AddOrUpdateParticipant(const RoomParticipantView& participant)
	{
		for (auto& existing : m_participants)
		{
			if (existing.id == participant.id)
			{
				existing = participant;
				Touch();
				return;
			}
		}
		m_participants.push_back(participant);
		Touch();
	}

	void RemoveParticipant(uint32 id)
	{
		const size_t removedCount = std::erase_if(
			m_participants, [id](const RoomParticipantView& participant) { return participant.id == id; }
		);
		if (removedCount > 0)
		{
			Touch();
		}
	}

	void SetParticipantReady(uint32 id, FB_ENUMS::PARTICIPANT_STATE_TYPE state)
	{
		if (auto* participant = FindParticipant(id))
		{
			participant->state = state;
			Touch();
		}
	}

	void SetHost(uint32 id)
	{
		if (auto* participant = FindParticipant(id))
		{
			participant->type = FB_ENUMS::PARTICIPANT_TYPE_HOST;
			participant->state = FB_ENUMS::PARTICIPANT_STATE_TYPE_READY;
			Touch();
		}
	}

	void SetParticipantTeam(uint32 id, FB_ENUMS::TEAM_TYPE team)
	{
		if (auto* participant = FindParticipant(id))
		{
			participant->team = team;
			Touch();
		}
	}

	void SetStatusMessage(std::string message)
	{
		m_message = std::move(message);
		Touch();
	}

	void SetStartPending(bool pending)
	{
		m_startPending = pending;
		Touch();
	}

	const std::vector<LobbyRoomView>&		GetRooms() const { return m_rooms; }
	size_t									GetUserCount() const { return m_usersByID.size(); }
	const std::vector<RoomParticipantView>& GetParticipants() const { return m_participants; }
	const std::string&						GetStatusMessage() const { return m_message; }
	uint32									GetLocalParticipantID() const { return m_localParticipantID; }
	bool									IsStartPending() const { return m_startPending; }
	uint64									GetRevision() const { return m_revision; }
	size_t GetLobbyPage() const { return std::min(m_lobbyPage, GetLobbyPageCount() - 1); }
	size_t GetLobbyPageCount() const
	{
		return std::max<size_t>(1, (m_rooms.size() + kRoomsPerPage - 1) / kRoomsPerPage);
	}

	const RoomParticipantView* GetLocalParticipant() const
	{
		for (const auto& participant : m_participants)
		{
			if (participant.id == m_localParticipantID)
			{
				return &participant;
			}
		}
		return nullptr;
	}

private:
	RoomParticipantView* FindParticipant(uint32 id)
	{
		for (auto& participant : m_participants)
		{
			if (participant.id == id)
			{
				return &participant;
			}
		}
		return nullptr;
	}

	void Touch() { ++m_revision; }

	std::vector<LobbyRoomView>				  m_rooms;
	std::unordered_map<uint16, size_t>		  m_roomIndexByID;
	std::unordered_map<uint32, LobbyUserView> m_usersByID;
	std::vector<RoomParticipantView>		  m_participants;
	std::string								  m_message;
	uint32									  m_localParticipantID = 0;
	uint64									  m_revision = 0;
	size_t									  m_lobbyPage = 0;
	bool									  m_startPending = false;
};
