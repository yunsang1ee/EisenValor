#pragma once

namespace Server {
	namespace Contents {
		class IDataTable {
		public:
			virtual bool LoadFromCSV(const std::string_view filePath) abstract;
		};
	}
}

