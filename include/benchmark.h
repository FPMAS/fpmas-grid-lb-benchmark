#include "fpmas.h"
#include "fpmas/utils/perf.h"
#include "output.h"

class LoadBalancingProbeTask : public fpmas::api::scheduler::Task {
	private:
		fpmas::api::scheduler::Task& lb_task;
		fpmas::utils::perf::Monitor& monitor;

		fpmas::utils::perf::Probe distribute_probe {"DISTRIBUTE"};

	public:

		LoadBalancingProbeTask(
				fpmas::api::scheduler::Task& lb_task,
				fpmas::utils::perf::Monitor& monitor
				) : lb_task(lb_task), monitor(monitor) {
		}

		void run() override;

};

class LoadBalancingProbe : public fpmas::api::model::LoadBalancing {
	private:
		fpmas::api::graph::LoadBalancing<fpmas::model::AgentPtr>& lb;

		fpmas::utils::perf::Probe balance_probe {"BALANCE"};

		fpmas::model::detail::LoadBalancingTask lb_task;
		LoadBalancingProbeTask lb_probe_task;

	public:
		fpmas::scheduler::Job job;
		fpmas::utils::perf::Monitor monitor;

		LoadBalancingProbe(
				fpmas::api::model::AgentGraph& graph,
				fpmas::api::graph::LoadBalancing<fpmas::model::AgentPtr>& lb
				) : lb(lb), lb_task(graph, *this), lb_probe_task(lb_task, monitor) {
			job.setBeginTask(lb_probe_task);
		}

		fpmas::graph::PartitionMap balance(
				fpmas::graph::NodeMap<fpmas::model::AgentPtr> node_map,
				fpmas::api::graph::PartitionMode mode) override;

		fpmas::graph::PartitionMap balance(
				fpmas::graph::NodeMap<fpmas::model::AgentPtr> node_map
				) override {
			return this->balance(node_map, fpmas::api::graph::PARTITION);
		}
};

class TestCase {
	private:
		IdleBehavior cell_behavior;
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
		TestCase(
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

