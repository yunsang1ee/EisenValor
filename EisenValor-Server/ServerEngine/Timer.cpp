#include "pch.h"
#include "Timer.h"

ServerEngine::Timer::Timer()
	:m_startTime{high_resolution_clock::now()}
{

}