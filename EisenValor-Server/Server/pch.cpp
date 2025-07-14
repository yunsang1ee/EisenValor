#include "pch.h"

#include "ClientSession.h"

std::shared_ptr<Server::ClientSession> MakeClientSessionFunc()
{
	// std::cout << "MakeSession!" << std::endl;
	return std::make_shared<Server::ClientSession>();
}