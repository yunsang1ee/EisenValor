#pragma once

//namespace Server {
//	namespace Contents {
//		class GameObject;
//		using GenFunc = std::shared_ptr<GameObject>(*)();
//		extern inline constinit std::array<GenFunc, static_cast<uint8>(GAME_OBJECT_TYPE::END)> GGenFuncs{};
//
//		class GameObjectFactory {
//		private:
//			GameObjectFactory() = delete;
//			~GameObjectFactory() = delete;
//
//			GameObjectFactory(const GameObjectFactory&) = delete;
//			GameObjectFactory(GameObjectFactory&&) = delete;
//
//			GameObjectFactory& operator=(const GameObjectFactory&) = delete;
//			GameObjectFactory& operator=(GameObjectFactory&&) = delete;
//		
//		public:
//			static void Init() noexcept
//			{
//				for(auto& func : GGenFuncs)
//					func = GenInvalid;
//
//				GGenFuncs[static_cast<uint8>(GAME_OBJECT_TYPE::GENERAL)] = GenGeneral;
//			}
//			static std::shared_ptr<GameObject> Create(GAME_OBJECT_TYPE type) noexcept;
//
//		private:
//			static std::shared_ptr<GameObject> GenInvalid();
//			static std::shared_ptr<GameObject> GenGeneral();
//		};
//	}
//}


