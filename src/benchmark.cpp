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
		std::string lb_algorithm_name,
		std::size_t grid_width, std::size_t grid_height, float occupation_rate,
		fpmas::api::scheduler::Scheduler& scheduler,
		fpmas::api::runtime::Runtime& runtime,
		fpmas::api::model::LoadBalancing& lb_algorithm
		) : lb_algorithm(lb_algorithm_name), model(scheduler, runtime, lb_algorithm), lb_probe(model.graph(), lb_algorithm), csv_output(*this) {
		auto& cell_group = model.buildGroup(0, cell_behavior);
		auto& agent_group = model.buildMoveGroup(1, move_behavior);

		//fpmas::model::GridCellFactory<fpmas::api::model::GridCell> cell_factory;
		fpmas::model::MooreGrid<>::Builder grid(grid_width, grid_height);

		grid.build(model, {cell_group});

		fpmas::model::UniformGridAgentMapping mapping(
				grid_width, grid_height, grid_width * grid_height * occupation_rate
				);
		fpmas::model::GridAgentBuilder<> agent_builder;
		fpmas::model::DefaultSpatialAgentFactory<BenchmarkAgent> agent_factory;

		agent_builder.build(model, {agent_group}, agent_factory, mapping);

		model.graph().synchronize();

		scheduler.schedule(0, 50, lb_probe.job);
		scheduler.schedule(0.1, 1, cell_group.jobs());
		scheduler.schedule(0.2, 1, agent_group.jobs());
		scheduler.schedule(0.3, 1, csv_output.job());
}

LoadBalancingCsvOutput::LoadBalancingCsvOutput(TestCase& test_case)
	:
		fpmas::io::FileOutput(test_case.lb_algorithm + ".%r.csv", test_case.model.getMpiCommunicator().getRank()),
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


