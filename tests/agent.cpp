#include "agent.h"
#include "gmock/gmock.h"

using namespace testing;

TEST(MetaAgent, datapack) {
	std::deque<DistributedId> contacts = {{0, 10}, {3, 4}, {12, 0}};

	fpmas::api::model::AgentPtr agent_ptr(new MetaGridAgent(contacts));
	fpmas::io::datapack::ObjectPack pack = agent_ptr;

	fpmas::api::model::AgentPtr unserial_agent = pack.get<fpmas::api::model::AgentPtr>();

	ASSERT_THAT(
			static_cast<const MetaGridAgent*>(unserial_agent.get())->contacts(),
			ElementsAreArray(contacts)
			);
			
}
