#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define TBB_PREVIEW_MEMORY_POOL 1

#define SINGLETON(classname)	\
private:						\
classname() { }			\
~classname(){ } 		\
friend class Singleton; 

#define MANAGER(classname) (classname::GetInstance())