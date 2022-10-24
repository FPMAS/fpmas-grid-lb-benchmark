#include "probe.h"

void GraphBalanceProbe::run() {
	graph_balance_probe.start();
	lb_task.run();
	graph_balance_probe.stop();
}

fpmas::graph::PartitionMap LoadBalancingProbe::balance(
		fpmas::graph::NodeMap<fpmas::model::AgentPtr> node_map,
		fpmas::api::graph::PartitionMode mode) {
	lb_algorithm_probe.start();
	auto result = lb.balance(node_map, mode);
	lb_algorithm_probe.stop();

	return result;
}

SyncProbeTask::SyncProbeTask(
		fpmas::api::utils::perf::Probe& sync_probe,
		fpmas::api::model::AgentGraph& graph)
	: sync_probe(sync_probe), sync_task(graph) {
	}

void SyncProbeTask::run() {
	sync_probe.start();
	sync_task.run();
	sync_probe.stop();
}

