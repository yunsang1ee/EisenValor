#pragma once

namespace Server {
	namespace Contents {
		class GameObject {
		private:
			Identity	m_identity;
			Transform	m_transform;
			Attribute	m_attribute;

		public:
			explicit GameObject();
			virtual ~GameObject() = default;
			
		public:		
			uint32 GetID() const noexcept { return m_identity.id; }
		};
	}

}


