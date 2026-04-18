#pragma once

#include "Component.h"

namespace GameServer {
	namespace Contents {
		class Script : public Component {
		public:
			Script() = default;
			virtual ~Script() = default;

		public:
			void SetName(const std::string_view name) { m_name = name; }
			const std::string& GetName() const { return m_name; }

		private:
			std::string m_name;

		};
	}
}