#pragma once

#include "fpmas/model/spatial/graph_builder.h"

#include "output.h"
#include "dot.h"
#include "probe.h"

class BasicMetaModel {
	public:
		virtual std::string getName() const = 0;
		virtual fpmas::api::model::Model& getModel() = 0;
		virtual fpmas::api::model::AgentGroup& cellGroup() = 0;
		virtual fpmas::api::model::AgentGroup& agentGroup() = 0;
		virtual DotOutput& getDotOutput() = 0;

		virtual BasicMetaModel* init() = 0;
		virtual void run() = 0;

		virtual ~BasicMetaModel() {
		}
};

template<typename BaseModel, typename CellType, typename AgentType>
class MetaModel : public BasicMetaModel {
	private:
		// Cell behaviors
		fpmas::model::IdleBehavior idle_behavior;
		Behavior<MetaSpatialCell> cell_update_edge_weights_behavior {
			&MetaSpatialCell::update_edge_weights
		};
		Behavior<MetaSpatialCell> cell_read_all_cell_behavior {
			&MetaSpatialCell::read_all_cell
		};
		Behavior<MetaSpatialCell> cell_write_all_cell_behavior {
			&MetaSpatialCell::write_all_cell
		};
		Behavior<MetaSpatialCell> cell_read_one_cell_behavior {
			&MetaSpatialCell::read_one_cell
		};
		Behavior<MetaSpatialCell> cell_write_one_cell_behavior {
			&MetaSpatialCell::write_one_cell
		};
		Behavior<MetaSpatialCell> cell_read_all_write_one_cell_behavior {
			&MetaSpatialCell::read_all_write_one_cell
		};
		Behavior<MetaSpatialCell> cell_read_all_write_all_cell_behavior {
			&MetaSpatialCell::read_all_write_all_cell
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
		BaseModel model;

	private:
		std::string name;
		fpmas::utils::perf::Monitor monitor;

		fpmas::utils::perf::Probe balance_probe {"BALANCE"};
		fpmas::utils::perf::Probe sync_probe {"SYNC"};
		fpmas::utils::perf::Probe distribute_probe {"DISTRIBUTE"};

		LoadBalancingProbe lb_probe;
		SyncProbeTask sync_probe_task;

		LoadBalancingCsvOutput csv_output;
		CellsOutput cells_output;
		AgentsOutput agents_output;
		DotOutput dot_output;
		BenchmarkConfig config;

	protected:
		virtual void buildCells(const BenchmarkConfig& config) = 0;
		virtual void buildAgents(const BenchmarkConfig& config) = 0;

	public:
		MetaModel(
				std::string name, BenchmarkConfig config,
				fpmas::api::scheduler::Scheduler& scheduler,
				fpmas::api::runtime::Runtime& runtime,
				fpmas::api::model::LoadBalancing& lb_algorithm,
				fpmas::scheduler::TimeStep lb_period
				);

		MetaModel<BaseModel, CellType, AgentType>* init() override;

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

template<typename BaseModel, typename CellType, typename AgentType>
MetaModel<BaseModel, CellType, AgentType>::MetaModel(
		std::string name, BenchmarkConfig config,
		fpmas::api::scheduler::Scheduler& scheduler,
		fpmas::api::runtime::Runtime& runtime,
		fpmas::api::model::LoadBalancing& lb_algorithm,
		fpmas::scheduler::TimeStep lb_period
		) :
	name(name),
	model(scheduler, runtime, lb_algorithm),
	lb_probe(balance_probe, distribute_probe, model.graph(), lb_algorithm),
	csv_output(
			*this,
			balance_probe,
			distribute_probe,
			ReaderWriter<CellType, CellType>::local_read_probe,
			ReaderWriter<CellType, CellType>::local_write_probe,
			ReaderWriter<CellType, CellType>::distant_read_probe,
			ReaderWriter<CellType, CellType>::distant_write_probe,
			sync_probe,
			monitor),
	sync_probe_task(sync_probe, model.graph()),
	cells_output(*this, this->name, config.grid_width, config.grid_height),
	agents_output(*this, this->name, config.grid_width, config.grid_height),
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

	
		scheduler.schedule(0, lb_period, lb_probe.job);
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
		scheduler.schedule(0.30, 1, csv_output.commit_probes_job);
		scheduler.schedule(0.31, 1, csv_output.job());
		scheduler.schedule(0.32, 1, csv_output.clear_monitor_job);

		fpmas::scheduler::TimeStep last_lb_date
			= ((config.num_steps-1) / lb_period) * lb_period;
		// Clears distant nodes
		if(config.json_output && config.environment == Environment::GRID) {
			if(config.json_output_period > 0) {
				// JSON cell output
				scheduler.schedule(0.33, config.json_output_period, cells_output.job());
				// JSON agent output
				if(config.occupation_rate > 0.0)
					scheduler.schedule(0.34, config.json_output_period, agents_output.job());

			} else {
				// JSON cell output
				scheduler.schedule(last_lb_date + 0.02, cells_output.job());
				// JSON agent output
				if(config.occupation_rate > 0.0)
					scheduler.schedule(last_lb_date + 0.03, agents_output.job());
			}

		}
		if(config.dot_output)
			// Dot output
			scheduler.schedule(last_lb_date + 0.04, dot_output.job());
}

template<typename BaseModel, typename CellType, typename AgentType>
MetaModel<BaseModel, CellType, AgentType>* MetaModel<BaseModel, CellType, AgentType>::init() {
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

template<template<typename> class SyncMode>
class MetaGridModel :
	public MetaModel<
		GridModel<SyncMode, MetaGridCell>,
		MetaGridCell, MetaGridAgent
	> {
		public:
			using MetaModel<
				GridModel<SyncMode, MetaGridCell>,
				MetaGridCell, MetaGridAgent
					>::MetaModel;

			void buildCells(const BenchmarkConfig& config) override;
			void buildAgents(const BenchmarkConfig& config) override;
};

template<template<typename> class SyncMode>
void MetaGridModel<SyncMode>::buildCells(const BenchmarkConfig& config) {
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
		dump_grid(config.grid_width, config.grid_height, local_cells);
}

template<template<typename> class SyncMode>
void MetaGridModel<SyncMode>::buildAgents(const BenchmarkConfig& config) {
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

template<template<typename> class SyncMode>
class MetaGraphModel :
	public MetaModel<
		SpatialModel<SyncMode, MetaGraphCell>,
		MetaGraphCell, MetaGraphAgent
	> {
		public:
			using MetaModel<
				SpatialModel<SyncMode, MetaGraphCell>,
				MetaGraphCell, MetaGraphAgent
					>::MetaModel;

			void buildCells(const BenchmarkConfig& config) override;
			void buildAgents(const BenchmarkConfig& config) override;
	};

template<template<typename> class SyncMode>
void MetaGraphModel<SyncMode>::buildCells(const BenchmarkConfig& config) {
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
	SpatialGraphBuilder<MetaGraphCell> graph_builder(
			*builder, config.num_cells,
			graph_cell_factory
			);
	fpmas::api::model::GroupList cell_groups;
	if(config.dynamic_cell_edge_weights)
		cell_groups.push_back(this->model.getGroup(UPDATE_CELL_EDGE_WEIGHTS_GROUP));
	if(config.cell_interactions != Interactions::NONE)
		cell_groups.push_back(this->model.getGroup(CELL_GROUP));
	graph_builder.build(this->model, cell_groups);

	delete builder;

	GraphRange<MetaGraphCell>::synchronize(this->model);
}

template<template<typename> class SyncMode>
void MetaGraphModel<SyncMode>::buildAgents(const BenchmarkConfig& config) {
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

struct MetaModelFactory {
	private:
		Environment environment;
		SyncMode sync_mode;

	public:
		MetaModelFactory(Environment environment, SyncMode sync_mode);

		BasicMetaModel* build(
				std::string lb_algorithm_name, BenchmarkConfig config,
				fpmas::api::scheduler::Scheduler& scheduler,
				fpmas::api::runtime::Runtime& runtime,
				fpmas::api::model::LoadBalancing& lb_algorithm,
				fpmas::scheduler::TimeStep lb_period
				);
};

