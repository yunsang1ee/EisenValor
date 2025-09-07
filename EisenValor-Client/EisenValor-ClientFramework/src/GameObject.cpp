#include "stdafxClientFramework.h"
#include "GameObject.h"


void GameObject::Handle_SC_MOVE(
	const Vec3& pos, const Vec3& rot, const Vec3& velocity, const Vec3& accel, const uint64 timeStamp
)
{
#ifdef DEAD_RECKONING
	lastServerPosition = pos;
	lastServerVelocity = velocity;
	lastServerAcceleration = accel;
	lastServerTimestamp = timeStamp;
	m_rot = rot;
	m_velocity = velocity;
	keyup = false;
#else
	lastServerPosition = pos;
	lastServerVelocity = velocity;
	lastServerRotation = rot;
	// std::cout << "Handle_SC_MOVE" << std::endl;
#endif
}
