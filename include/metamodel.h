#pragma once

#include "fpmas/model/spatial/graph_builder.h"

#include "output.h"
#include "dot.h"
#include "probe.h"

/**
 * @file metamodel.h
 * Contains MetaModel related features.
 */

/**
 * Generic MetaModel interface, without template.
 */
class BasicMetaModel {
	public:
		/**
		 * Name of the model.
		 */
		virtual std::string getName() const = 0;
		/**
		 * Generic model instance.
		 */
		virtual fpmas::api::model::Model& getModel() = 0;
		/**
		 * Agent group containing the Cell network.
		 */
		virtual fpmas::api::model::AgentGroup& cellGroup() = 0;
		/**
		 * Agent group containing Spatial Agents.
		 */
		virtual fpmas::api::model::AgentGroup& agentGroup() = 0;
		/**
		 * Can be used to perform a model output in a DOT file.
		 */
		virtual DotOutput& getDotOutput() = 0;

		/**
		 * Initializes the Cell network and Spatial Agents.
		 */
		virtual BasicMetaModel* init() = 0;
		/**
		 * Runs the MetaModel.
		 */
		virtual void run() = 0;

		virtual ~BasicMetaModel() {
		}
};

/**
 * Generic MetaModel implementation, with features that are common for both
 * MetaGraphModel and MetaGridModel. The MetaModel class cannot be used on its
 * own.
 *
 * @tparam BaseModel The concrete underlying spatial model type, such as
 * SpatialModel<...> or GridModel<...>
 * @tparam AgentType The concrete type of Spatial Agents, such as
 * SpatialAgent<...> or GridAgent<...>
 */
template<typename BaseModel, typename AgentType>
class MetaModel : public BasicMetaModel {
	private:
		typedef typename BaseModel::CellType CellType;

		// Cell behaviors
		fpmas::model::IdleBehavior idle_behavior;
		Behavior<MetaCell> cell_update_edge_weights_behavior {
			&MetaCell::update_edge_weights
		};
		Behavior<MetaCell> cell_read_all_cell_behavior {
			&MetaCell::read_all_cell
		};
		Behavior<MetaCell> cell_write_all_cell_behavior {
			&MetaCell::write_all_cell
		};
		Behavior<MetaCell> cell_read_one_cell_behavior {
			&MetaCell::read_one_cell
		};
		Behavior<MetaCell> cell_write_one_cell_behavior {
			&MetaCell::write_one_cell
		};
		Behavior<MetaCell> cell_read_all_write_one_cell_behavior {
			&MetaCell::read_all_write_one_cell
		};
		Behavior<MetaCell> cell_read_all_write_all_cell_behavior {
			&MetaCell::read_all_write_all_cell
		};

		// Agent behaviors
		Behavior<AgentType> create_relations_from_neighborhood {
			&AgentType::create_relations_from_neighborhood
		};
		Behavior<AgentType> create_relations_from_contacts {
			&AgentType::create_relations_from_contacts
		};
		Behavior<AgentType> handle_new_contacts {
			&AgentType::handle_new_contacts
		};
		Behavior<AgentType> move_behavior {
			&AgentType::move
		};

		fpmas::scheduler::detail::LambdaTask sync_graph_task {
				[this] () {this->model.graph().synchronize();}
				};
		fpmas::scheduler::Job sync_graph {{sync_graph_task}};

	protected:
		/**
		 * Spatial model instance.
		 */
		BaseModel model;

	private:
		std::string name;
		fpmas::utils::perf::Monitor monitor;

		fpmas::utils::perf::Probe lb_algorithm_probe {"LB_ALGORITHM"};
		fpmas::utils::perf::Probe sync_probe {"SYNC"};
		fpmas::utils::perf::Probe graph_balance_probe {"GRAPH_BALANCE"};

		GraphBalanceProbe graph_balance_probe_job;
		SyncProbeTask sync_probe_task;

		MetaModelCsvOutput csv_output;
		CellsLocationOutput cells_location_output;
		CellsUtilityOutput cells_utility_output;
		AgentsOutput agents_output;
		DotOutput dot_output;
		ModelConfig config;

	protected:
		/**
		 * Method used to build the Cell network.
		 *
		 * @param config Model configuration
		 */
		virtual void buildCells(const ModelConfig& config) = 0;
		/**
		 * Method used to build Spatial Agents and place them on the Cell
		 * network.
		 *
		 * @param config Model configuration
		 */
		virtual void buildAgents(const ModelConfig& config) = 0;

	public:
		/**
		 * MetaModel constructor.
		 *
		 * @param name Name of the model
		 * @param config Model configuration
		 * @param scheduler Scheduler on which agent execution, load balancing
		 * and outputs are planned
		 * @param runtime Runtime used to execute the MetaModel
		 * @param lb_algorithm Load balancing algorithm to apply to the
		 * MetaModel
		 * @param lb_period Period at which the load balancing algorithm should
		 * be applied, starting at time step 0
		 */
		MetaModel(
				std::string name, ModelConfig config,
				fpmas::api::scheduler::Scheduler& scheduler,
				fpmas::api::runtime::Runtime& runtime,
				fpmas::api::model::LoadBalancing& lb_algorithm,
				fpmas::scheduler::TimeStep lb_period
				);

		// BasicMetaModel implementation //

		MetaModel<BaseModel, AgentType>* init() override;

		void run() override {
			model.runtime().run(config.num_steps);
		}

		std::string getName() const override {
			return name;
		}

		fpmas::api::model::Model& getModel() override {
			return model;
		}

		fpmas::api::model::AgentGroup& cellGroup() override {
			return model.cellGroup();
		}

		fpmas::api::model::AgentGroup& agentGroup() override {
			return model.getGroup(AGENT_GROUP);
		}

		DotOutput& getDotOutput() override {
			return dot_output;
		}
};

template<typename BaseModel, typename AgentType>
MetaModel<BaseModel, AgentType>::MetaModel(
		std::string name, ModelConfig config,
		fpmas::api::scheduler::Scheduler& scheduler,
		fpmas::api::runtime::Runtime& runtime,
		fpmas::api::model::LoadBalancing& lb_algorithm,
		fpmas::scheduler::TimeStep lb_period
		) :
	name(name),
	model(scheduler, runtime, lb_algorithm),
	graph_balance_probe_job(
			model.graph(), lb_algorithm, lb_algorithm_probe, graph_balance_probe),
	csv_output(
			*this,
			lb_algorithm_probe,
			graph_balance_probe,
			ReaderWriter::local_read_probe,
			ReaderWriter::local_write_probe,
			ReaderWriter::distant_read_probe,
			ReaderWriter::distant_write_probe,
			sync_probe,
			monitor),
	sync_probe_task(sync_probe, model.graph()),
	cells_location_output(*this, this->name, config.grid_width, config.grid_height),
	cells_utility_output(*this, config.grid_width, config.grid_height),
	agents_output(*this, config.grid_width, config.grid_height),
	dot_output(*this, this->name + ".%t"),
	config(config) {
		switch(config.cell_interactions) {
			case Interactions::READ_ALL:
				model.buildGroup(
						CELL_GROUP,
						cell_read_all_cell_behavior
						);
				break;
			case Interactions::WRITE_ALL:
				model.buildGroup(
						CELL_GROUP,
						cell_write_all_cell_behavior
						);
				break;
			case Interactions::READ_ONE:
				model.buildGroup(
						CELL_GROUP,
						cell_read_one_cell_behavior
						);
				break;
			case Interactions::WRITE_ONE:
				model.buildGroup(
						CELL_GROUP,
						cell_write_one_cell_behavior
						);
				break;
			case Interactions::READ_ALL_WRITE_ONE:
				model.buildGroup(
						CELL_GROUP,
						cell_read_all_write_one_cell_behavior
						);
				break;
			case Interactions::READ_ALL_WRITE_ALL:
				model.buildGroup(
						CELL_GROUP,
						cell_read_all_write_all_cell_behavior
						);
				break;
			default:
				model.buildGroup(
						CELL_GROUP,
						idle_behavior
						);
				break;
		}
		auto& create_relations_neighbors_group = model.buildGroup(
				RELATIONS_FROM_NEIGHBORS_GROUP, create_relations_from_neighborhood
				);
		auto& create_relations_contacts_group = model.buildGroup(
				RELATIONS_FROM_CONTACTS_GROUP, create_relations_from_contacts
				);
		auto& handle_new_contacts_group = model.buildGroup(
				HANDLE_NEW_CONTACTS_GROUP, handle_new_contacts
				);
		auto& move_group = model.buildMoveGroup(
				MOVE_GROUP, move_behavior
				);

	
		scheduler.schedule(0, lb_period, graph_balance_probe_job.job);
		if(config.occupation_rate > 0.0) {
			if(config.agent_interactions == AgentInteractions::CONTACTS) {
				scheduler.schedule(
						0.20, config.refresh_local_contacts,
						create_relations_neighbors_group.jobs()
						);
				scheduler.schedule(
						0.21, config.refresh_distant_contacts,
						create_relations_contacts_group.jobs()
						);
				scheduler.schedule(
						0.22, config.refresh_distant_contacts,
						handle_new_contacts_group.jobs()
						);
			}
			scheduler.schedule(0.23, 1, move_group.jobs());
		}
		if(config.dynamic_cell_edge_weights) {
			auto& update_cell_edge_weights_group = model.buildGroup(
					UPDATE_CELL_EDGE_WEIGHTS_GROUP,
					cell_update_edge_weights_behavior);
			scheduler.schedule(0.24, 1, update_cell_edge_weights_group.jobs());
		}
		
		if(config.cell_interactions != Interactions::NONE) {
			model.getGroup(CELL_GROUP).agentExecutionJob().setEndTask(sync_probe_task);
			scheduler.schedule(0.25, 1, model.getGroup(CELL_GROUP).jobs());
		}
		scheduler.schedule(0.30, 1, csv_output.jobs());

		fpmas::scheduler::TimeStep last_lb_date
			= ((config.num_steps-1) / lb_period) * lb_period;
		// Clears distant nodes
		if(config.json_output && config.environment == Environment::GRID) {
			if(config.json_output_period > 0) {
				// JSON cell output
				scheduler.schedule(0.33, config.json_output_period, cells_location_output.job());
				// JSON agent output
				if(config.occupation_rate > 0.0)
					scheduler.schedule(0.34, config.json_output_period, agents_output.job());

			} else {
				// If json_output_period <= 0, the json output is only performed
				// at the end of the simulation.

				// JSON cell output
				scheduler.schedule(last_lb_date + 0.02, cells_location_output.job());
				// JSON agent output
				if(config.occupation_rate > 0.0)
					scheduler.schedule(last_lb_date + 0.03, agents_output.job());
			}

		}
		if(config.dot_output)
			// Dot output
			scheduler.schedule(last_lb_date + 0.04, dot_output.job());
}

template<typename BaseModel, typename AgentType>
MetaModel<BaseModel, AgentType>* MetaModel<BaseModel, AgentType>::init() {
	buildCells(config);
	model.graph().synchronize();

	buildAgents(config);
	// Static node weights
	for(auto cell : model.cellGroup().localAgents())
		cell->node()->setWeight(config.cell_weight);
	for(auto agent : model.getGroup(AGENT_GROUP).localAgents())
		agent->node()->setWeight(config.agent_weight);

	model.graph().synchronize();

	return this;
}

/**
 * A generic MetaModel extension where Spatial Agents are moving on a Moore grid.
 */
template<template<typename> class SyncMode>
class MetaGridModel :
	public MetaModel<GridModel<SyncMode, MetaGridCell>, MetaGridAgent> {
		public:
			using MetaModel<GridModel<SyncMode, MetaGridCell>, MetaGridAgent>
				::MetaModel;

			/**
			 * Builds a grid of size `config.grid_width*config.grid_height`.
			 *
			 * A utility is assigned to each cell, according to the
			 * `config.utility` value:
			 * - Utility::UNIFORM: UniformUtility
			 * - Utility::LINEAR: LinearUtility
			 * - Utility::INVERSE: InverseUtility
			 * - Utility::STEP: StepUtility
			 *
			 * GridAttractors are defined from `config.grid_attractors`. See
			 * MetaGridCell factory for more detailed information.
			 *
			 * If config.json_output is true, a `grid.json` file describing
			 * the utility of Cells is built.
			 *
			 * @param config Model configuration
			 */
			void buildCells(const ModelConfig& config) override;

			/**
			 * Builds GridAgents on the grid.
			 *
			 * A total of `grid_size*config.occupation_rate` Agents are
			 * initialized randomly and uniformly on the grid.
			 *
			 * @param config Model configuration
			 */
			void buildAgents(const ModelConfig& config) override;
};

template<template<typename> class SyncMode>
void MetaGridModel<SyncMode>::buildCells(const ModelConfig& config) {
	std::unique_ptr<UtilityFunction> utility_function;
	switch(config.utility) {
		case Utility::UNIFORM:
			utility_function.reset(new UniformUtility);
			break;
		case Utility::LINEAR:
			utility_function.reset(new LinearUtility);
			break;
		case Utility::INVERSE:
			utility_function.reset(new InverseUtility);
			break;
		case Utility::STEP:
			utility_function.reset(new StepUtility);
			break;
	}
	MetaGridCellFactory cell_factory(
			*utility_function, config.grid_attractors, config.cell_size);
	MooreGrid<MetaGridCell>::Builder grid(
			cell_factory, config.grid_width, config.grid_height);
	fpmas::api::model::GroupList cell_groups;
	if(config.cell_interactions != Interactions::NONE)
		cell_groups.push_back(this->model.getGroup(CELL_GROUP));
	auto local_cells = grid.build(this->model, cell_groups);
	if(config.json_output)
		CellsUtilityOutput(*this, config.grid_width, config.grid_height)
			.dump();
}

template<template<typename> class SyncMode>
void MetaGridModel<SyncMode>::buildAgents(const ModelConfig& config) {
	fpmas::model::UniformGridAgentMapping mapping(
			config.grid_width, config.grid_height,
			config.grid_width * config.grid_height * config.occupation_rate
			);
	fpmas::model::GridAgentBuilder<MetaGridCell> agent_builder;
	fpmas::model::DefaultSpatialAgentFactory<MetaGridAgent> agent_factory;

	agent_builder.build(
			this->model,
			{
			this->model.getGroup(RELATIONS_FROM_NEIGHBORS_GROUP),
			this->model.getGroup(RELATIONS_FROM_CONTACTS_GROUP),
			this->model.getGroup(HANDLE_NEW_CONTACTS_GROUP),
			this->model.getGroup(MOVE_GROUP)
			},
			agent_factory, mapping);
}

/**
 * A generic MetaModel extension where Spatial Agents are moving on an arbitrary
 * graph.
 *
 * Agents randomly move to out neighbors of their location cells.
 */
template<template<typename> class SyncMode>
class MetaGraphModel :
	public MetaModel<SpatialModel<SyncMode, MetaGraphCell>, MetaGraphAgent> {
		public:
			using MetaModel<SpatialModel<SyncMode, MetaGraphCell>, MetaGraphAgent>
				::MetaModel;

			/**
			 * Builds a graph according to the specified `config.environment`:
			 * - SMALL_WORLD
			 * - RANDOM
			 * - CLUSTERED
			 * In any case, `config.num_cells` nodes are built, with an average
			 * output degree of `config.output_degree`.
			 *
			 * The parameter `config.p` is passed to the SMALL_WORLD graph
			 * builder to determine the proportion of edges to relink in the
			 * Small-World build process.
			 *
			 * @param config Model configuration
			 */
			void buildCells(const ModelConfig& config) override;

			/**
			 * Builds GraphAgents on the spatial graph.
			 *
			 * A total of
			 * `num_cells*config.occupation_rate` agents are randomly and
			 * uniformly initialized on the spatial graph.
			 *
			 * @param config Model configuration
			 */
			void buildAgents(const ModelConfig& config) override;
	};

template<template<typename> class SyncMode>
void MetaGraphModel<SyncMode>::buildCells(const ModelConfig& config) {
	fpmas::random::PoissonDistribution<std::size_t> edge_dist(config.output_degree);
	fpmas::api::graph::DistributedGraphBuilder<fpmas::model::AgentPtr>* builder;
	switch(config.environment) {
		case Environment::RANDOM:
			builder = new DistributedUniformGraphBuilder(edge_dist);
			break;
		case Environment::CLUSTERED:
			builder = new DistributedClusteredGraphBuilder(edge_dist);
			break;
		case Environment::SMALL_WORLD:
			builder = new SmallWorldGraphBuilder(config.p, config.output_degree);
			break;
		default:
			// Grid type
			break;
	}
	MetaGraphCellFactory graph_cell_factory(config.cell_size);
	CellNetworkBuilder<MetaGraphCell> cell_network_builder(
			*builder, config.num_cells,
			graph_cell_factory
			);
	fpmas::api::model::GroupList cell_groups;
	if(config.dynamic_cell_edge_weights)
		cell_groups.push_back(this->model.getGroup(UPDATE_CELL_EDGE_WEIGHTS_GROUP));
	if(config.cell_interactions != Interactions::NONE)
		cell_groups.push_back(this->model.getGroup(CELL_GROUP));
	cell_network_builder.build(this->model, cell_groups);

	delete builder;

	GraphRange<MetaGraphCell>::synchronize(this->model);
}

template<template<typename> class SyncMode>
void MetaGraphModel<SyncMode>::buildAgents(const ModelConfig& config) {
	fpmas::model::UniformAgentMapping mapping(
			this->getModel().getMpiCommunicator(),
			this->cellGroup(),
			config.num_cells * config.occupation_rate
			);
	fpmas::model::SpatialAgentBuilder<MetaGraphCell> agent_builder;
	fpmas::model::DefaultSpatialAgentFactory<MetaGraphAgent> agent_factory;
	agent_builder.build(
			this->model,
			{
			this->model.getGroup(RELATIONS_FROM_NEIGHBORS_GROUP),
			this->model.getGroup(RELATIONS_FROM_CONTACTS_GROUP),
			this->model.getGroup(HANDLE_NEW_CONTACTS_GROUP),
			this->model.getGroup(MOVE_GROUP)
			},
			agent_factory, mapping);
}

/**
 * A factory class used to instantiate MetaGridModel and MetaGraphCell instances
 * depending on the specified Environment and SyncMode.
 */
struct MetaModelFactory {
	private:
		Environment environment;
		SyncMode sync_mode;

	public:
		/**
		 * MetaModelFactory constructor.
		 *
		 * @param environment Environment type of the built MetaModel
		 * @param sync_mode Synchronization mode used by the built MetaModel
		 */
		MetaModelFactory(Environment environment, SyncMode sync_mode);

		/**
		 * Builds a MetaModel instance from the specified parameters.
		 *
		 * @param name Name of the model
		 * @param config Model configuration
		 * @param scheduler Scheduler instance used by the MetaModel to plan
		 * agent execution
		 * @param runtime Runtime instance used to execute the MetaModel
		 * @param lb_algorithm Load balancing algorithm to apply to the
		 * MetaModel
		 * @param lb_period Period at which the load balancing algorithm should
		 * be scheduled, starting at time step 0
		 */
		BasicMetaModel* build(
				std::string name, ModelConfig config,
				fpmas::api::scheduler::Scheduler& scheduler,
				fpmas::api::runtime::Runtime& runtime,
				fpmas::api::model::LoadBalancing& lb_algorithm,
				fpmas::scheduler::TimeStep lb_period
				);
};

