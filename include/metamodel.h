#pragma once

#include "output.h"
#include "probe.h"

class BasicMetaModel {
	public:
		virtual std::string getLoadBalancingAlgorithmeName() const = 0;
		virtual LoadBalancingProbe& getLoadBalancingProbe() = 0;
		virtual fpmas::api::model::Model& getModel() = 0;
		virtual fpmas::api::model::AgentGroup& cellGroup() = 0;
		virtual fpmas::api::model::AgentGroup& agentGroup() = 0;

		virtual BasicMetaModel* init() = 0;
		virtual void run() = 0;

		virtual ~BasicMetaModel() {
		}
};

template<typename BaseModel, typename AgentType>
class MetaModel : public BasicMetaModel {
	private:
		Behavior<MetaGridCell> cell_behavior {
			&MetaGridCell::update_edge_weights
		};
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


	public:
		BaseModel model;
		std::string lb_algorithm_name;
		LoadBalancingProbe lb_probe;

	private:
		LoadBalancingCsvOutput csv_output;
		CellsOutput cells_output;
		AgentsOutput agents_output;
		BenchmarkConfig config;

	protected:
		virtual void buildCells(const BenchmarkConfig& config) = 0;
		virtual void buildAgents(const BenchmarkConfig& config) = 0;

	public:
		MetaModel(
				std::string lb_algorithm_name, BenchmarkConfig config,
				fpmas::api::scheduler::Scheduler& scheduler,
				fpmas::api::runtime::Runtime& runtime,
				fpmas::api::model::LoadBalancing& lb_algorithm,
				fpmas::scheduler::TimeStep lb_period
				);

		MetaModel<BaseModel, AgentType>* init() override;

		void run() override {
			model.runtime().run(config.num_steps);
		}

		std::string getLoadBalancingAlgorithmeName() const override {
			return lb_algorithm_name;
		}

		LoadBalancingProbe& getLoadBalancingProbe() override {
			return lb_probe;
		}

		fpmas::api::model::Model& getModel() override {
			return model;
		}

		fpmas::api::model::AgentGroup& cellGroup() override {
			return model.cellGroup();
		}

		fpmas::api::model::AgentGroup& agentGroup() override {
			return model.getGroup(MOVE_GROUP);
		}
};

template<typename BaseModel, typename AgentType>
MetaModel<BaseModel, AgentType>::MetaModel(
		std::string lb_algorithm_name, BenchmarkConfig config,
		fpmas::api::scheduler::Scheduler& scheduler,
		fpmas::api::runtime::Runtime& runtime,
		fpmas::api::model::LoadBalancing& lb_algorithm,
		fpmas::scheduler::TimeStep lb_period
		) :
	lb_algorithm_name(lb_algorithm_name + "-" + std::to_string(lb_period)),
	model(scheduler, runtime, lb_algorithm),
	lb_probe(model.graph(), lb_algorithm), csv_output(*this),
	cells_output(model, this->lb_algorithm_name, config.grid_width, config.grid_height),
	agents_output(model, this->lb_algorithm_name, config.grid_width, config.grid_height),
	config(config) {
		auto& cell_group = model.buildGroup(CELL_GROUP, cell_behavior);
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
		scheduler.schedule(0.24, 1, cell_group.jobs());
		scheduler.schedule(0.3, 1, csv_output.job());

		fpmas::scheduler::TimeStep last_lb_date
			= ((config.num_steps-1) / lb_period) * lb_period;
		// Clears distant nodes
		scheduler.schedule(last_lb_date + 0.01, sync_graph);
		// JSON cell output
		scheduler.schedule(last_lb_date + 0.02, cells_output.job());
		// JSON agent output
		scheduler.schedule(last_lb_date + 0.03, agents_output.job());
}

template<typename BaseModel, typename AgentType>
MetaModel<BaseModel, AgentType>* MetaModel<BaseModel, AgentType>::init() {
	buildCells(config);
	model.graph().synchronize();

	buildAgents(config);
	// Static node weights
	for(auto cell : model.getGroup(CELL_GROUP).localAgents())
		cell->node()->setWeight(config.cell_weight);
	for(auto agent : model.getGroup(MOVE_GROUP).localAgents())
		agent->node()->setWeight(config.agent_weight);

	model.graph().synchronize();

	return this;
}

class MetaGridModel :
	public MetaModel<
		GridModel<fpmas::synchro::GhostMode, MetaGridCell>,
		MetaGridAgent
	> {
		public:
			using MetaModel<
				GridModel<fpmas::synchro::GhostMode, MetaGridCell>,
				MetaGridAgent
					>::MetaModel;

			void buildCells(const BenchmarkConfig& config) override;
			void buildAgents(const BenchmarkConfig& config) override;
};

class MetaGraphModel :
	public MetaModel<
		SpatialModel<fpmas::synchro::GhostMode, MetaGraphCell>,
		MetaGraphAgent
	> {
		using MetaModel<
			SpatialModel<fpmas::synchro::GhostMode, MetaGraphCell>,
			MetaGraphAgent
				>::MetaModel;

			void buildCells(const BenchmarkConfig& config) override;
			void buildAgents(const BenchmarkConfig& config) override;
	};

struct BasicMetaModelFactory {
	virtual BasicMetaModel* build(
			std::string lb_algorithm_name, BenchmarkConfig config,
			fpmas::api::scheduler::Scheduler& scheduler,
			fpmas::api::runtime::Runtime& runtime,
			fpmas::api::model::LoadBalancing& lb_algorithm,
			fpmas::scheduler::TimeStep lb_period
			) = 0;

	virtual ~BasicMetaModelFactory() {
	}
};

template<typename MetaModelType>
	struct MetaModelFactory : public BasicMetaModelFactory {
		MetaModelType* build(
				std::string lb_algorithm_name, BenchmarkConfig config,
				fpmas::api::scheduler::Scheduler& scheduler,
				fpmas::api::runtime::Runtime& runtime,
				fpmas::api::model::LoadBalancing& lb_algorithm,
				fpmas::scheduler::TimeStep lb_period
				) override {
			return new MetaModelType(
					lb_algorithm_name, config, scheduler, runtime,
					lb_algorithm, lb_period
					);
		}
	};

