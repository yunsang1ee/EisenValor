#pragma once
#include "DBConnection.h"

#include "Singleton.hpp"

class DBConnectionPool : public Singleton<DBConnectionPool> {
	SINGLETON(DBConnectionPool)
public:
	bool					Connect(int32 connectionCount, const WCHAR* connectionString);
	void					Clear();

	DBConnection*			Pop();
	void					Push(DBConnection* connection);

private:
	std::mutex					m_mutex;
	SQLHENV						m_environment = SQL_NULL_HANDLE;
	std::vector<DBConnection*>	m_connections;
};