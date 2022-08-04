#include "gtest/gtest.h"
#include "metamodel.h"

FPMAS_BASE_DATAPACK_SET_UP(
		GridCell::JsonBase,
		MetaGridAgent::JsonBase,
		MetaGridCell::JsonBase,
		MetaGraphCell::JsonBase,
		MetaGraphAgent::JsonBase
		);

int main(int argc, char **argv) {
	FPMAS_REGISTER_AGENT_TYPES(
			GridCell::JsonBase,
			MetaGridAgent::JsonBase,
			MetaGridCell::JsonBase,
			MetaGraphCell::JsonBase,
			MetaGraphAgent::JsonBase
			);
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
