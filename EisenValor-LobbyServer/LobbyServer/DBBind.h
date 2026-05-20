#pragma once
#include "DBConnection.h"

template<int32 C>
struct FullBits { enum { value = (1 << (C - 1)) | FullBits<C - 1>::value }; };

template<>
struct FullBits<1> { enum { value = 1 }; };

template<>
struct FullBits<0> { enum { value = 0 }; };

template<int32 ParamCount/*SQL쿼리 안의 ? 개수*/, int32 ColumnCount/*SELECT결과로 받을 컬럼 개수*/>
class DBBind {
public:
	DBBind(DBConnection& dbConnection, const WCHAR* query)
		: m_dbConnection(dbConnection), m_query(query)
	{
		::memset(m_paramIndex, 0, sizeof(m_paramIndex));
		::memset(m_columnIndex, 0, sizeof(m_columnIndex));
		m_paramFlag = 0;
		m_columnFlag = 0;
		dbConnection.Unbind();
	}

	bool Validate()
	{
		return m_paramFlag == FullBits<ParamCount>::value && m_columnFlag == FullBits<ColumnCount>::value;
	}

	bool Execute()
	{
		ASSERT_CRASH(Validate());
		return m_dbConnection.Execute(m_query);
	}

	bool Fetch()
	{
		return m_dbConnection.Fetch();
	}

public:
	template<typename T>
	void BindParam(int32 idx, T& value)
	{
		m_dbConnection.BindParam(idx + 1, &value, &m_paramIndex[idx]);
		m_paramFlag |= (1LL << idx);
	}

	void BindParam(int32 idx, const WCHAR* value)
	{
		m_dbConnection.BindParam(idx + 1, value, &m_paramIndex[idx]);
		m_paramFlag |= (1LL << idx);
	}

	void BindParam(int32 idx, const char* value)
	{
		m_dbConnection.BindParam(idx + 1, value, &m_paramIndex[idx]);
		m_paramFlag |= (1LL << idx);
	}

	template<typename T, int32 N>
	void BindParam(int32 idx, T(&value)[N])
	{
		m_dbConnection.BindParam(idx + 1, (const BYTE*)value, size32(T) * N, &m_paramIndex[idx]);
		m_paramFlag |= (1LL << idx);
	}

	template<typename T>
	void BindParam(int32 idx, T* value, int32 N)
	{
		m_dbConnection.BindParam(idx + 1, (const BYTE*)value, size32(T) * N, &m_paramIndex[idx]);
		m_paramFlag |= (1LL << idx);
	}

	template<typename T>
	void BindCol(int32 idx, T& value)
	{
		m_dbConnection.BindCol(idx + 1, &value, &m_columnIndex[idx]);
		m_columnFlag |= (1LL << idx);
	}

	template<int32 N>
	void BindCol(int32 idx, WCHAR(&value)[N])
	{
		m_dbConnection.BindCol(idx + 1, value, N - 1, &m_columnIndex[idx]);
		m_columnFlag |= (1LL << idx);
	}

	void BindCol(int32 idx, WCHAR* value, int32 len)
	{
		m_dbConnection.BindCol(idx + 1, value, len - 1, &m_columnIndex[idx]);
		m_columnFlag |= (1LL << idx);
	}

	template<typename T, int32 N>
	void BindCol(int32 idx, T(&value)[N])
	{
		m_dbConnection.BindCol(idx + 1, value, size32(T) * N, &m_columnIndex[idx]);
		m_columnFlag |= (1LL << idx);
	}

protected:
	DBConnection&	m_dbConnection;
	const WCHAR*	m_query;
	SQLLEN			m_paramIndex[ParamCount > 0 ? ParamCount : 1];
	SQLLEN			m_columnIndex[ColumnCount > 0 ? ColumnCount : 1];
	uint64			m_paramFlag;
	uint64			m_columnFlag;
};

