#pragma once

#include "Component.h"

namespace Server {
	namespace Contents {
		class Script : public Component {
		private:
			std::string m_name;
		
		public:
			Script();
			virtual ~Script();

		public:
			void SetName(const std::string_view name) { m_name = name; }
			const std::string_view GetName() const { return m_name; }
		};
	}
}

