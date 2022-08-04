#pragma once

#include "yaml-cpp/yaml.h"
#include "fpmas/api/scheduler/scheduler.h"
#include "fpmas/api/model/spatial/grid.h"
#include "fpmas/utils/macros.h"

FPMAS_DEFINE_GROUPS(
		RELATIONS_FROM_NEIGHBORS_GROUP,
		RELATIONS_FROM_CONTACTS_GROUP,
		HANDLE_NEW_CONTACTS_GROUP,
		MOVE_GROUP,
		CELL_GROUP);
#define AGENT_GROUP MOVE_GROUP

FPMAS_DEFINE_LAYERS(CONTACT, NEW_CONTACT);

enum class Environment {
	GRID, GRAPH
};

enum class GraphType {
	RANDOM, CLUSTERED, SMALL_WORLD
};

enum class Utility {
	UNIFORM, LINEAR, INVERSE, STEP
};

enum class MovePolicy {
	RANDOM, MAX
};

enum class LbAlgorithm {
	SCHEDULED_LB, ZOLTAN_LB, GRID_LB, ZOLTAN_CELL_LB, RANDOM_LB
};

enum class AgentInteractions {
	LOCAL, CONTACTS
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
		bool is_valid = true;

		Environment environment;
		GraphType graph_type;
		std::size_t num_cells;
		std::size_t output_degree;
		std::size_t grid_width;
		std::size_t grid_height;
		float occupation_rate;
		fpmas::api::scheduler::TimeStep num_steps;
		Utility utility;
		AgentInteractions agent_interactions;
		float cell_weight;
		float agent_weight;
		fpmas::api::scheduler::TimeStep refresh_local_contacts;
		fpmas::api::scheduler::TimeStep refresh_distant_contacts;
		std::vector<Attractor> attractors;
		std::vector<TestCaseConfig> test_cases;

		BenchmarkConfig(std::string config_file);

	private:
		template<typename T>
			void load_config(
					std::string field_name, T& target, YAML::Node node,
					std::string type_name
					) {
				if(!node.IsDefined()) {
					std::cerr <<
						"[FATAL ERROR] Missing configuration field: " + field_name +
						" (" + type_name + ")"
						<< std::endl;
					this->is_valid = false;
				} else {
					try{
						target = node.as<T>();
					} catch (const YAML::TypedBadConversion<T>& e) {
						this->is_valid = false;
						std::cerr <<
							"[FATAL ERROR] Bad " + field_name + " field parsing. "
							"Expected type: " + type_name
							<< std::endl;
					}
				}
			}
};

namespace YAML {
	template<>
		struct convert<Environment> {
			static Node encode(const Environment& rhs);
			static bool decode(const Node& node, Environment& rhs);
		};

	template<>
		struct convert<GraphType> {
			static Node encode(const GraphType& graph_type);
			static bool decode(const Node& node, GraphType& graph_type);
		};

	template<>
		struct convert<Utility> {
			static Node encode(const Utility& rhs);
			static bool decode(const Node& node, Utility& rhs);
		};

	template<>
		struct convert<MovePolicy> {
			static Node encode(const MovePolicy& rhs);
			static bool decode(const Node& node, MovePolicy& rhs);
		};

	template<>
		struct convert<LbAlgorithm> {
			static Node encode(const LbAlgorithm& rhs);
			static bool decode(const Node& node, LbAlgorithm& rhs);
		};

	template<>
		struct convert<AgentInteractions> {
			static Node encode(const AgentInteractions& rhs);
			static bool decode(const Node& node, AgentInteractions& rhs);
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

