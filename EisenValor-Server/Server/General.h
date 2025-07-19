#pragma once

#include "GameObject.h"

namespace Server {
	namespace Contents {
		class General : public GameObject {
			// Session
		public:
			General();
			virtual ~General() = default;
		};
	}
}


