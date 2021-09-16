#include "yaml-cpp/yaml.h"
#include "fpmas/api/scheduler/scheduler.h"
#include "fpmas/api/model/spatial/grid.h"
#include "fpmas/utils/macros.h"

FPMAS_DEFINE_GROUPS(AGENT_GROUP, CELL_GROUP);

enum Utility {
	UNIFORM, LINEAR, INVERSE, STEP
};

enum LbAlgorithm {
	SCHEDULED_LB, ZOLTAN_LB, GRID_LB, ZOLTAN_CELL_LB, RANDOM_LB
};

struct Attractor {
	fpmas::api::model::DiscretePoint center;
	float radius;
};

struct TestCaseConfig {
	LbAlgorithm algorithm;
	std::vector<fpmas::api::scheduler::TimeStep> lb_periods;
};

class BenchmarkConfig {
	public:
		std::size_t grid_width;
		std::size_t grid_height;
		float occupation_rate;
		fpmas::api::scheduler::TimeStep num_steps;
		Utility utility;
		std::vector<Attractor> attractors;
		std::map<LbAlgorithm, std::vector<fpmas::api::scheduler::TimeStep>> test_cases;

		BenchmarkConfig(std::string config_file);
};

namespace YAML {
	template<>
		struct convert<Utility> {
			static Node encode(const Utility& rhs);
			static bool decode(const Node& node, Utility& rhs);
		};

	template<>
		struct convert<LbAlgorithm> {
			static Node encode(const LbAlgorithm& rhs);
			static bool decode(const Node& node, LbAlgorithm& rhs);
		};

	template<>
		struct convert<Attractor> {
			static Node encode(const Attractor& rhs);
			static bool decode(const Node& node, Attractor& rhs);
		};

	template<>
		struct convert<TestCaseConfig> {
			static Node encode(const TestCaseConfig& rhs);
			static bool decode(const Node& node, TestCaseConfig& rhs);
		};
}

