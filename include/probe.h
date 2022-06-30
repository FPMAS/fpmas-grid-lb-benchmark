#pragma once

#include "fpmas.h"
#include "fpmas/utils/perf.h"

class LoadBalancingProbeTask : public fpmas::scheduler::Task {
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

