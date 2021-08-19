#include "yaml-cpp/yaml.h"
#include "fpmas/api/scheduler/scheduler.h"
#include "fpmas/api/model/spatial/grid.h"

enum CellDistribution {
	UNIFORM, CLUSTERED
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
		CellDistribution cell_distribution;
		std::vector<Attractor> attractors;
		std::map<LbAlgorithm, std::vector<fpmas::api::scheduler::TimeStep>> test_cases;

		BenchmarkConfig(std::string config_file);
};

namespace YAML {
	template<>
		struct convert<CellDistribution> {
			static Node encode(const CellDistribution& rhs);
			static bool decode(const Node& node, CellDistribution& rhs);
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

