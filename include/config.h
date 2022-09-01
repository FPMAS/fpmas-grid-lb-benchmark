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
		UPDATE_CELL_EDGE_WEIGHTS_GROUP,
		CELL_GROUP);
#define AGENT_GROUP MOVE_GROUP

FPMAS_DEFINE_LAYERS(CONTACT, NEW_CONTACT);

#define LOAD_YAML_CONFIG_0(FIELD_NAME, TYPENAME)\
	load_config(#FIELD_NAME, FIELD_NAME, config[#FIELD_NAME], #TYPENAME)
#define LOAD_YAML_CONFIG_0_OPTIONAL(FIELD_NAME, TYPENAME, DEFAULT)\
	load_config_optional(#FIELD_NAME, FIELD_NAME, config[#FIELD_NAME], #TYPENAME, DEFAULT)
#define LOAD_YAML_CONFIG_1(ROOT, FIELD_NAME, TYPENAME)\
	load_config(#ROOT "::" #FIELD_NAME, ROOT::FIELD_NAME, config[#ROOT][#FIELD_NAME], #TYPENAME)
#define LOAD_YAML_CONFIG_1_OPTIONAL(ROOT, FIELD_NAME, TYPENAME, DEFAULT)\
	load_config_optional(#ROOT "::" #FIELD_NAME, ROOT::FIELD_NAME, config[#ROOT][#FIELD_NAME], #TYPENAME, DEFAULT)


enum class Environment {
	GRID, RANDOM, CLUSTERED, SMALL_WORLD
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

enum class Interactions {
	NONE, READ_ALL, READ_ONE, READ_ALL_WRITE_ONE, READ_ALL_WRITE_ALL, WRITE_ALL, WRITE_ONE
};

enum class SyncMode {
	GHOST_MODE, GLOBAL_GHOST_MODE, HARD_SYNC_MODE
};

struct Attractor {
	float radius;
};

struct GridAttractor : public Attractor {
	fpmas::api::model::DiscretePoint center;
};

struct TestCaseConfig {
	LbAlgorithm algorithm;
	std::vector<fpmas::api::scheduler::TimeStep> lb_periods;
};

struct GraphConfig {
	bool is_valid = true;

	Environment environment;
	std::size_t grid_width;
	std::size_t grid_height;
	std::size_t num_cells;
	std::size_t output_degree;
	float p;
	float cell_weight = 1.0f;
	bool dynamic_cell_edge_weights = false;
	Utility utility = Utility::UNIFORM;
	std::vector<Attractor> attractors;
	std::vector<GridAttractor> grid_attractors;
	Interactions cell_interactions = Interactions::NONE;
	SyncMode sync_mode = SyncMode::GHOST_MODE;
	bool json_output = false;
	bool dot_output = false;

	template<typename T>
		void load_config_optional(
				std::string field_name, T& target, YAML::Node node,
				std::string type_name, const T& default_value) {
			if(node.IsDefined()) {
				try{
					target = node.as<T>();
				} catch (const YAML::TypedBadConversion<T>& e) {
					this->is_valid = false;
					std::cerr <<
						"[FATAL ERROR] Bad " + field_name + " field parsing. "
						"Expected type: " + type_name
						<< std::endl;
				}
			} else {
				target = default_value;
			}
		}

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

	GraphConfig(YAML::Node config);
};

struct BenchmarkConfig : public GraphConfig {
	float occupation_rate;
	fpmas::api::scheduler::TimeStep num_steps;
	AgentInteractions agent_interactions = AgentInteractions::LOCAL;
	float agent_weight = 1.0f;
	fpmas::api::scheduler::TimeStep refresh_local_contacts;
	fpmas::api::scheduler::TimeStep refresh_distant_contacts;
	std::vector<TestCaseConfig> test_cases;

	BenchmarkConfig(const GraphConfig& graph_config);
	BenchmarkConfig(YAML::Node config);
};

namespace YAML {
	template<>
		struct convert<Environment> {
			static Node encode(const Environment& graph_type);
			static bool decode(const Node& node, Environment& graph_type);
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
		struct convert<Interactions> {
			static Node encode(const Interactions& rhs);
			static bool decode(const Node& node, Interactions& rhs);
		};

	template<>
		struct convert<SyncMode> {
			static Node encode(const SyncMode& rhs);
			static bool decode(const Node& node, SyncMode& rhs);
		};

	template<>
		struct convert<Attractor> {
			static Node encode(const Attractor& rhs);
			static bool decode(const Node& node, Attractor& rhs);
		};

	template<>
		struct convert<GridAttractor> {
			static Node encode(const GridAttractor& rhs);
			static bool decode(const Node& node, GridAttractor& rhs);
		};

	template<>
		struct convert<TestCaseConfig> {
			static Node encode(const TestCaseConfig& rhs);
			static bool decode(const Node& node, TestCaseConfig& rhs);
		};
}

