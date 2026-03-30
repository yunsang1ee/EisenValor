#include "pch.h"
#include "Timer.h"

GameServerEngine::Timer::Timer()
	:m_startTime{high_resolution_clock::now()}
{

}