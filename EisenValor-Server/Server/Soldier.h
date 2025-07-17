#pragma once

#include "GameObject.h"

namespace Server {
	namespace Contents{
		class Soldier : public GameObject {
			Soldier();
			virtual ~Soldier() = default;
		};
	}
}