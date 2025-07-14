#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define SINGLETON(classname)	\
private:						\
classname()noexcept { }			\
~classname()noexcept  { } 		\
friend class Singleton; 

#define MANAGER(classname) (classname::GetInstance())

#define RIO_EXT_FUNC_TB MANAGER(ServerEngine::RIOCore)->GetRioExtFuncTB()