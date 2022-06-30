#include "probe.h"

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

