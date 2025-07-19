#include "pch.h"
#include "RecvBuffer.h"

#include "RIOCore.h"

ServerEngine::RecvBuffer::RecvBuffer(const uint32 bufferSize)
	:RIOBuffer(bufferSize)
{
}

ServerEngine::RecvBuffer::~RecvBuffer()
{

}