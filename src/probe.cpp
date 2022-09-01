#include "probe.h"

void LoadBalancingProbeTask::run() {
	distribute_probe.start();
	lb_task.run();
	distribute_probe.stop();
}

fpmas::graph::PartitionMap LoadBalancingProbe::balance(
		fpmas::graph::NodeMap<fpmas::model::AgentPtr> node_map,
		fpmas::api::graph::PartitionMode mode) {
	balance_probe.start();
	auto result = lb.balance(node_map, mode);
	balance_probe.stop();

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

