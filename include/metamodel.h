#pragma once

#include "output.h"
#include "probe.h"

class MetaModel {
	private:
		Behavior<BenchmarkCell> cell_behavior {
			&BenchmarkCell::update_edge_weights
		};
		Behavior<BenchmarkAgent> create_relations_from_neighborhood {
			&BenchmarkAgent::create_relations_from_neighborhood
		};
		Behavior<BenchmarkAgent> create_relations_from_contacts {
			&BenchmarkAgent::create_relations_from_contacts
		};
		Behavior<BenchmarkAgent> handle_new_contacts {
			&BenchmarkAgent::handle_new_contacts
		};
		Behavior<BenchmarkAgent> move_behavior {
			&BenchmarkAgent::move
		};

		fpmas::scheduler::detail::LambdaTask sync_graph_task {
				[this] () {this->model.graph().synchronize();}
				};
		fpmas::scheduler::Job sync_graph {{sync_graph_task}};


	public:
		GridModel<fpmas::synchro::GhostMode, BenchmarkCell> model;
		std::string lb_algorithm_name;
		LoadBalancingProbe lb_probe;

	private:
		LoadBalancingCsvOutput csv_output;
		CellsOutput cells_output;
		AgentsOutput agents_output;
		BenchmarkConfig config;

	public:
		MetaModel(
				std::string lb_algorithm_name, BenchmarkConfig config,
				fpmas::api::scheduler::Scheduler& scheduler,
				fpmas::api::runtime::Runtime& runtime,
				fpmas::api::model::LoadBalancing& lb_algorithm,
				fpmas::scheduler::TimeStep lb_period
				);

		void run() {
			model.runtime().run(config.num_steps);
		}
};

