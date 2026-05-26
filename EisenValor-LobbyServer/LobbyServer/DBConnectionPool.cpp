#include "pch.h"
#include "DBConnectionPool.h"

bool DBConnectionPool::Connect(int32 connectionCount, const WCHAR* connectionString)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if(::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_environment) != SQL_SUCCESS)
		return false;

	if(::SQLSetEnvAttr(m_environment, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0) != SQL_SUCCESS)
		return false;

	for(int32 i = 0; i < connectionCount; i++) {
		DBConnection* connection = new DBConnection();
		if(connection->Connect(m_environment, connectionString) == false)
			return false;

		m_connections.push_back(connection);
	}

	return true;
}

void DBConnectionPool::Clear()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if(m_environment != SQL_NULL_HANDLE) {
		::SQLFreeHandle(SQL_HANDLE_ENV, m_environment);
		m_environment = SQL_NULL_HANDLE;
	}

	for(DBConnection* connection : m_connections)
		delete connection;

	m_connections.clear();
}

DBConnection* DBConnectionPool::Pop()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if(m_connections.empty())
		return nullptr;

	DBConnection* connection = m_connections.back();
	m_connections.pop_back();
	return connection;
}

void DBConnectionPool::Push(DBConnection* connection)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_connections.push_back(connection);
}
