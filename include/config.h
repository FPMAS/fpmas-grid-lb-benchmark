#pragma once

#include "yaml-cpp/yaml.h"
#include "fpmas/api/scheduler/scheduler.h"
#include "fpmas/api/model/spatial/grid.h"
#include "fpmas/utils/macros.h"

/**
 * @file config.h
 * Configuration features.
 */

/**
 * Defines AgentGroup ids used internally by the MetaModel.
 */
FPMAS_DEFINE_GROUPS(
		RELATIONS_FROM_NEIGHBORS_GROUP,
		RELATIONS_FROM_CONTACTS_GROUP,
		HANDLE_NEW_CONTACTS_GROUP,
		MOVE_GROUP,
		UPDATE_CELL_EDGE_WEIGHTS_GROUP,
		CELL_GROUP);
/**
 * MOVE_GROUP alias, since all SpatialAgents and only them are contained in the
 * MOVE_GROUP AgentGroup.
 */
#define AGENT_GROUP MOVE_GROUP

/**
 * Defines layer ids used internally by the MetaModel.
 */
FPMAS_DEFINE_LAYERS(CONTACT, NEW_CONTACT);

/**
 * Environment type.
 */
enum class Environment {
	/**
	 * A regular grid.
	 *
	 * @see [MooreGridBuilder](https://fpmas.github.io/FPMAS/classfpmas_1_1model_1_1MooreGridBuilder.html)
	 */
	GRID,
	/**
	 * An uniform random graph.
	 *
	 * @see [UniformGraphBuilder](https://fpmas.github.io/FPMAS/classfpmas_1_1graph_1_1DistributedUniformGraphBuilder.html)
	 */
	RANDOM,
	/**
	 * A random graph with a high clustering coefficient.
	 *
	 * @see [ClusteredGraphBuilder](https://fpmas.github.io/FPMAS/classfpmas_1_1graph_1_1DistributedClusteredGraphBuilder.html)
	 */
	CLUSTERED,
	/**
	 * A [Small-World](https://en.wikipedia.org/wiki/Small-world_network) graph.
	 *
	 * @see [SmallWorldGraphBuilder](https://fpmas.github.io/FPMAS/classfpmas_1_1graph_1_1SmallWorldGraphBuilder.html)
	 */
	SMALL_WORLD
};

/**
 * Shape of utility of Cells. Currently only available for Environment::GRID.
 */
enum class Utility {
	/**
	 * All cells have the same utility.
	 *
	 * @see UniformUtility
	 */
	UNIFORM,
	/**
	 * Utility decreases linearly from the center of attraction to the radius.
	 *
	 * @see LinearUtility
	 */
	LINEAR,
	/**
	 * Utility decreases from the center of attraction according to an inverse
	 * function.
	 *
	 * @see InverseUtility
	 */
	INVERSE,
	/**
	 * A high uniform step until radius, and then decreases using an inverse
	 * function.
	 *
	 * @see StepUtility
	 */
	STEP
};

/**
 * Agents move policy.
 */
enum class MovePolicy {
	/**
	 * Go to a random cell, with a random distribution consistent with utilities
	 * of cells.
	 *
	 * @see RandomMovePolicy
	 */
	RANDOM,
	/**
	 * Go to the cell with the maximum utility.
	 *
	 * @see MaxMovePolicy
	 */
	MAX
};

/**
 * Load balancing algorithms.
 */
enum class LbAlgorithm {
	/**
	 * A load balancing algorithm that applies Zoltan, taking into account the
	 * agent scheduling. Very costly and not efficient.
	 *
	 * @see [ScheduledLoadBalancing](https://fpmas.github.io/FPMAS/classfpmas_1_1graph_1_1ScheduledLoadBalancing.html)
	 */
	SCHEDULED_LB,
	/**
	 * Raw Zoltan load balancing.
	 *
	 * @see [ZoltanLoadBalancing](https://fpmas.github.io/FPMAS/classfpmas_1_1graph_1_1ZoltanLoadBalancing.html)
	 */
	ZOLTAN_LB,
	/**
	 * Grid based load balancing.
	 *
	 * @see [GridLoadBalancing](https://fpmas.github.io/FPMAS/classfpmas_1_1model_1_1GridLoadBalancing.html)
	 */
	GRID_LB,
	/**
	 * Zoltan applied only to the cell network, and agent assigned to the same
	 * process as their location.
	 *
	 * The Zoltan load balancing is applied to the cell network at each
	 * iteration.
	 *
	 * @see [CellLoadBalancing](https://fpmas.github.io/FPMAS/classfpmas_1_1model_1_1CellLoadBalancing.html)
	 */
	ZOLTAN_CELL_LB,
	/**
	 * Same as ZOLTAN_CELL_LB, but the Zoltan algorithm is only applied to the
	 * cell network the first time the algorithm is applied.
	 *
	 * @see [StaticCellLoadBalancing](https://fpmas.github.io/FPMAS/classfpmas_1_1model_1_1StaticCellLoadBalancing.html)
	 */
	STATIC_ZOLTAN_CELL_LB,
	/**
	 * Completely random load balancing. Not recommended.
	 *
	 * @see [RandomLoadBalancing](https://fpmas.github.io/FPMAS/classfpmas_1_1graph_1_1RandomLoadBalancing.html)
	 */
	RANDOM_LB
};

/**
 * Defines the agent interactions graph.
 */
enum class AgentInteractions {
	/**
	 * Agents only interact with agents in their geographical perception field.
	 */
	LOCAL,
	/**
	 * Same as LOCAL, but also maintains a list of distant contacts.
	 *
	 * @see MetaAgent
	 */
	CONTACTS
};

/**
 * Agent interactions scheme.
 *
 * Note that write operations can be handled properly only with
 * SyncMode::HARD_SYNC_MODE.
 */
enum class Interactions {
	/**
	 * No interaction.
	 */
	NONE,
	/**
	 * Performs a _read_ on all neighbors.
	 */
	READ_ALL,
	/**
	 * Performs a _read_ on a randomly selected neighbor.
	 */
	READ_ONE,
	/**
	 * Performs a _write_ on all neighbors.
	 */
	WRITE_ALL,
	/**
	 * Performs a _write_ on a randomly selected neighbor.
	 */
	WRITE_ONE,
	/**
	 * Performs a _read_ on all neighbors, and then performs a _write_ on a
	 * randomly selected neighbor.
	 */
	READ_ALL_WRITE_ONE,
	/**
	 * Performs a _read_ on all neighbors, and then performs a _write_ on all
	 * neighbors.
	 */
	READ_ALL_WRITE_ALL
};

/**
 * Synchronization modes.
 */
enum class SyncMode {
	/**
	 * Distant agents are read from a ghost copy. No write.
	 *
	 * @see [GhostMode](https://fpmas.github.io/FPMAS/namespacefpmas_1_1synchro.html#a6f1830e3d3961bb80925d771d313b277)
	 */
	GHOST_MODE,
	/**
	 * All agents are read from a ghost copy. No write.
	 *
	 * @see [GlobalGhostMode](https://fpmas.github.io/FPMAS/namespacefpmas_1_1synchro.html#a122c6433985ae734346737106256370b)
	 */
	GLOBAL_GHOST_MODE,
	/**
	 * Agents are read and written directly from distant processes, allowing
	 * concurrent read and write operations.
	 *
	 * @see
	 * [HardSyncModeWithGhostLink](https://fpmas.github.io/FPMAS/namespacefpmas_1_1synchro.html#a8cd51b213abd6169839e5fb6a6554d45)
	 */
	HARD_SYNC_MODE
};

/**
 * Grid and graph attractor base used to compute cell utilities. Only
 * Environment::GRID is currently supported.
 */
struct Attractor {
	/**
	 * Utility radius.
	 */
	float radius;
};

/**
 * GridAttractor extension: the center of the Attractor is defined as a discrete
 * point on the grid.
 */
struct GridAttractor : public Attractor {
	/**
	 * Coordinates of the center of the attractor.
	 */
	fpmas::api::model::DiscretePoint center;
};

/**
 * Configuration of a test case.
 *
 * The purpose of a test case is to test a load balancing algorithm with a given
 * period.
 */
struct TestCaseConfig {
	/**
	 * Load balancing algorithm to test.
	 */
	LbAlgorithm algorithm;
	/**
	 * Periods at which the load balancing algorithm should be tested.
	 */
	std::vector<fpmas::api::scheduler::TimeStep> lb_periods;
};

/**
 * Contains all the configuration relative to the environment of the model.
 *
 * Each attribute corresponds to a YAML field. Some might be optional.
 */
struct GraphConfig {
	/**
	 * True iff the configuration was properly loaded from the YAML file.
	 */
	bool is_valid = true;

	/**
	 * Environment type.
	 */
	Environment environment;
	/**
	 * For GRID environment, specifies the grid width.
	 */
	std::size_t grid_width;
	/**
	 * For GRID environment, specifies the grid height.
	 */
	std::size_t grid_height;
	/**
	 * For environments other than GRID, specifies the number of cells in the
	 * global graph.
	 */
	std::size_t num_cells;
	/**
	 * For environments other than GRID, specifies the average output degree of
	 * each cell in the cell network.
	 */
	std::size_t output_degree;
	/**
	 * For SMALL_WORLD environment, probability to relink each edge.
	 *
	 * The graph is regular for p=0, and completely random for p=1. p=0.1 is a
	 * decent value that is generally enough to generate the Small World
	 * property within the graph.
	 */
	float p;
	/**
	 * Weight of each cell.
	 */
	float cell_weight = 1.0f;
	/**
	 * Cells utility function.
	 */
	Utility utility = Utility::UNIFORM;
	/**
	 * List of GridAttractors. The utility of each cell corresponds to the sum
	 * of utilities generated by each attractor.
	 */
	std::vector<GridAttractor> grid_attractors;
	/**
	 * The Zoltan IMBALANCE_TOL parameter.
	 *
	 * @see https://sandialabs.github.io/Zoltan/ug_html/ug_alg.html#LB%20Parameters
	 */
	float zoltan_imbalance_tol = 1.1;
	/**
	 * If true, a json output representing the utility of each cell and the
	 * count of agents in each cells is generated.
	 */
	bool json_output = false;
	/**
	 * Period at which the json_output should be performed.
	 *
	 * If unspecified or negative but `json_output` is set to true, the json
	 * output is only performed at the end of the simulation.
	 */
	int json_output_period = -1;
	/**
	 * If true, a DOT file representing the global simulation graph is generated
	 * at the end of each model test case.
	 */
	bool dot_output = false;

	/**
	 * Loads an optional configuration field into the corresponding attribute.
	 *
	 * @param field_name Name of the YAML field
	 * @param target Reference to the attribute into which data should be loaded
	 * @param node YAML node containing field value
	 * @param type_name Name of the type, used to generate error messages
	 * @param default_value Default value of the optional field, used if node is
	 * actually not defined.
	 */
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

	/**
	 * Loads a mandatory configuration field into the corresponding attribute.
	 *
	 * @param field_name Name of the YAML field
	 * @param target Reference to the attribute into which data should be loaded
	 * @param node YAML node containing field value
	 * @param type_name Name of the type, used to generate error messages
	 */
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

	/**
	 * Loads the configuration from the specified YAML node.
	 *
	 * @param config YAML configuration
	 */
	GraphConfig(YAML::Node config);
};

/**
 * General MetaModel configuration.
 */
struct ModelConfig : public GraphConfig {
	/**
	 * Agent occupation rate. The total count of agents is set as `total cell
	 * count * occupation_rate`.
	 */
	float occupation_rate;
	/**
	 * Number of time steps to simulate.
	 */
	fpmas::api::scheduler::TimeStep num_steps;
	/**
	 * Type of agent interactions.
	 */
	AgentInteractions agent_interactions = AgentInteractions::LOCAL;
	/**
	 * Interactions between cells within the cell network. Spatial agent
	 * interactions is currently not supported, so this feature is more relevant
	 * in a pure graph model, where occupation_rate=0.
	 */
	Interactions cell_interactions = Interactions::NONE;
	/**
	 * If true, the weight of each edge in the cell network is incremented by
	 * the count of agents located in the cell at each iteration. This might be
	 * useful to reflect the DistributedMoveAlgorithm cost within the graph.
	 */
	bool dynamic_cell_edge_weights = false;
	/**
	 * Synchronization mode.
	 */
	SyncMode sync_mode = SyncMode::GHOST_MODE;
	/**
	 * Size of cells data, in bytes. This is useful to evaluate the evolution of
	 * the duration of read/write operations depending of the amount of data to
	 * transfer for each cell.
	 */
	std::size_t cell_size = 0;
	/**
	 * Agent weight.
	 */
	float agent_weight = 1.0f;
	/**
	 * If agent_interactions is CONTACTS, period at which new contacts are added
	 * from the current perceptions of the agent.
	 *
	 * @see MetaAgent::create_relations_neighbors_group()
	 */
	fpmas::api::scheduler::TimeStep refresh_local_contacts;
	/**
	 * If agent_interactions is CONTACTS, period at which contacts are built
	 * from two randomly selected contacts.
	 *
	 * @see MetaAgent::create_relations_from_contacts()
	 */
	fpmas::api::scheduler::TimeStep refresh_distant_contacts;
	/**
	 * List of test cases for the current set up. A new model is built and
	 * simulated for each case.
	 */
	std::vector<TestCaseConfig> test_cases;

	/**
	 * Builds a ModelConfig from the specified GraphConfig.
	 *
	 * All other fields are left to their default value.
	 *
	 * @param graph_config existing environment configuration
	 */
	ModelConfig(const GraphConfig& graph_config);
	/**
	 * Loads the configuration from the specified YAML node.
	 *
	 * @param config YAML configuration
	 */
	ModelConfig(YAML::Node config);
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

