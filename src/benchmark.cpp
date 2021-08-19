#include "benchmark.h"


void LoadBalancingProbeTask::run() {
	distribute_probe.start();
	lb_task.run();
	distribute_probe.stop();

	monitor.commit(distribute_probe);
}

fpmas::graph::PartitionMap LoadBalancingProbe::balance(
		fpmas::graph::NodeMap<fpmas::model::AgentPtr> node_map,
		fpmas::api::graph::PartitionMode mode) {
	balance_probe.start();
	
	auto result = lb.balance(node_map, mode);

	balance_probe.stop();
	monitor.commit(balance_probe);

	return result;
}

TestCase::TestCase(
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
		auto& cell_group = model.buildGroup(0, cell_behavior);
		auto& agent_group = model.buildMoveGroup(1, move_behavior);

		std::unique_ptr<fpmas::api::model::GridCellFactory<BenchmarkCell>> cell_factory;
		switch(config.cell_distribution) {
			case UNIFORM:
				cell_factory.reset(new UniformBenchmarkCellFactory);
				break;
			case CLUSTERED:
				cell_factory.reset(new ClusteredBenchmarkCellFactory(config.attractors));
				break;
		}
		fpmas::model::MooreGrid<BenchmarkCell>::Builder grid(
				*cell_factory, config.grid_width, config.grid_height);

		auto local_cells = grid.build(model, {cell_group});
		dump_grid(config.grid_width, config.grid_height, local_cells);

		fpmas::model::UniformGridAgentMapping mapping(
				config.grid_width, config.grid_height,
				config.grid_width * config.grid_height * config.occupation_rate
				);
		fpmas::model::GridAgentBuilder<BenchmarkCell> agent_builder;
		fpmas::model::DefaultSpatialAgentFactory<BenchmarkAgent> agent_factory;

		agent_builder.build(model, {agent_group}, agent_factory, mapping);

		model.graph().synchronize();

		scheduler.schedule(0, lb_period, lb_probe.job);
		scheduler.schedule(0.1, 1, cell_group.jobs());
		scheduler.schedule(0.2, 1, agent_group.jobs());
		scheduler.schedule(0.3, 1, csv_output.job());
		fpmas::scheduler::TimeStep last_lb_date = ((config.num_steps-1) / lb_period) * lb_period;
		scheduler.schedule(last_lb_date + 0.01, cells_output.job());
		scheduler.schedule(last_lb_date + 0.02, agents_output.job());
}

LoadBalancingCsvOutput::LoadBalancingCsvOutput(TestCase& test_case)
	:
		fpmas::io::FileOutput(test_case.lb_algorithm_name + ".%r.csv", test_case.model.getMpiCommunicator().getRank()),
		LbCsvOutput(*this,
			{"TIME", [&test_case] {return test_case.model.runtime().currentDate();}},
			{"BALANCE_TIME", [&test_case] {
			auto result = std::chrono::duration_cast<std::chrono::microseconds>(
					test_case.lb_probe.monitor.totalDuration("BALANCE")
					).count();
			return result;
			}},
			{"DISTRIBUTE_TIME", [&test_case] {
			auto result = std::chrono::duration_cast<std::chrono::microseconds>(
					test_case.lb_probe.monitor.totalDuration("DISTRIBUTE")
					).count();
			test_case.lb_probe.monitor.clear();
			return result;
			}},
			{"AGENTS", [&test_case] {
			float total_weight = 0;
			for(auto agent : test_case.model.getGroup(1).localAgents())
				total_weight += agent->node()->getWeight();
			return total_weight;
			}},
			{"CELLS", [&test_case] {
			float total_weight = 0;
			for(auto agent : test_case.model.getGroup(0).localAgents())
				total_weight += agent->node()->getWeight();
			return total_weight;
			}},
			{"DISTANT_AGENT_EDGES", [&test_case] {
			float total_weight = 0;
			for(auto agent : test_case.model.getGroup(1).localAgents())
				for(auto edge : agent->node()->getOutgoingEdges())
					if(edge->state() == fpmas::api::graph::DISTANT)
						total_weight+=edge->getWeight();
			return total_weight;
			}},
			{"DISTANT_CELL_EDGES", [&test_case] {
			float total_weight = 0;
			for(auto cell : test_case.model.getGroup(0).localAgents())
				for(auto edge : cell->node()->getOutgoingEdges())
					if(edge->state() == fpmas::api::graph::DISTANT)
						total_weight+=edge->getWeight();
			return total_weight;
			}}
			) {
		}


