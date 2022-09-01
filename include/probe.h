#pragma once

#include "fpmas.h"
#include "fpmas/utils/perf.h"

class LoadBalancingProbeTask : public fpmas::scheduler::Task {
	private:
		fpmas::api::scheduler::Task& lb_task;
		fpmas::api::utils::perf::Probe& distribute_probe;

	public:

		LoadBalancingProbeTask(
				fpmas::api::scheduler::Task& lb_task,
				fpmas::api::utils::perf::Probe& distribute_probe
				) : lb_task(lb_task), distribute_probe(distribute_probe) {
		}

		void run() override;

};

class LoadBalancingProbe : public fpmas::api::model::LoadBalancing {
	private:
		fpmas::api::utils::perf::Probe& balance_probe;
		fpmas::api::graph::LoadBalancing<fpmas::model::AgentPtr>& lb;

		fpmas::model::detail::LoadBalancingTask lb_task;
		LoadBalancingProbeTask lb_probe_task;

	public:
		fpmas::scheduler::Job job;

		LoadBalancingProbe(
				fpmas::utils::perf::Probe& balance_probe,
				fpmas::utils::perf::Probe& distribute_probe,
				fpmas::api::model::AgentGraph& graph,
				fpmas::api::graph::LoadBalancing<fpmas::model::AgentPtr>& lb
				) :
			balance_probe(balance_probe),
			lb(lb), lb_task(graph, *this),
			lb_probe_task(lb_task, distribute_probe) {
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

class SyncProbeTask : public fpmas::scheduler::Task {
	private:
		fpmas::api::utils::perf::Probe& sync_probe;
		fpmas::model::detail::SynchronizeGraphTask sync_task;

	public:
		SyncProbeTask(
				fpmas::api::utils::perf::Probe& sync_probe,
				fpmas::api::model::AgentGraph& graph);

		void run() override;
};

