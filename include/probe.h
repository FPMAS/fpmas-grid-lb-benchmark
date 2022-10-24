#pragma once

#include "fpmas.h"
#include "fpmas/utils/perf.h"

/**
 * @file probe.h
 * Contains features used to perform MetaModel time probes.
 */

/**
 * A wrapper for an existing LoadBalancing algorithm, that adds probes to
 * measure its execution time.
 */
class LoadBalancingProbe : public fpmas::api::model::LoadBalancing {
	private:
		// Probes only the load balancing algorithm execution time
		fpmas::api::utils::perf::Probe& lb_algorithm_probe;
		fpmas::api::model::LoadBalancing& lb;

	public:
		
		/**
		 * LoadBalancingProbe constructor.
		 *
		 * @param lb_algorithm_probe Probe that measures only the load balancing
		 * algorithm execution time
		 * @param lb Load balancing algorithm to probe
		 */
		LoadBalancingProbe(
				fpmas::api::utils::perf::Probe& lb_algorithm_probe,
				fpmas::api::model::LoadBalancing& lb
				) :
			lb_algorithm_probe(lb_algorithm_probe),
			lb(lb) {
		}

		/**
		 * Returns the results of the existing LoadBalancing algorithm,
		 * measuring only the LoadBalancing::balance() execution time, without
		 * including the distribution time.
		 *
		 * This method is automatically called by the FPMAS LoadBalancingTask.
		 *
		 * @param node_map nodes to balance
		 * @param mode Partition mode, PARTITION or REPARTITION
		 */
		fpmas::graph::PartitionMap balance(
				fpmas::graph::NodeMap<fpmas::model::AgentPtr> node_map,
				fpmas::api::graph::PartitionMode mode) override;

		/**
		 * Deprecated, implemented for compatibility with an old API.
		 */
		fpmas::graph::PartitionMap balance(
				fpmas::graph::NodeMap<fpmas::model::AgentPtr> node_map
				) override {
			return this->balance(node_map, fpmas::api::graph::PARTITION);
		}
};

/**
 * Task used to probe the total DistributedGraph::balance() time, that includes
 * the load balancing algorithm application **and** the distribution process.
 */
class GraphBalanceProbe : public fpmas::scheduler::Task {
	private:
		// Wrapper for the existing load balancing algorithm, that adds probes
		LoadBalancingProbe load_balancing_algorithm;
		fpmas::model::detail::LoadBalancingTask lb_task;
		// Probes the complete load balancing + distribution process
		fpmas::api::utils::perf::Probe& graph_balance_probe;

	public:
		/**
		 * Job that can be directly scheduled instead of
		 * Model::loabBalancingJob() in order to probe load balancing and
		 * distribution times.
		 */
		fpmas::scheduler::Job job;

		/**
		 * DistributeProbeTask constructor.
		 *
		 * @param graph Distributed graph to balance
		 * @param probed_lb Load balancing algorithm to apply and probe
		 * @param lb_algorithm_probe Probe used to measure only the load
		 * balancing algorithm execution time
		 * @param graph_balance_probe Probe used to measure the complete
		 * DistributedGraph::balance() execution time, including the
		 * distribution process
		 */
		GraphBalanceProbe(
				fpmas::api::model::AgentGraph& graph,
				fpmas::api::model::LoadBalancing& probed_lb,
				fpmas::api::utils::perf::Probe& lb_algorithm_probe,
				fpmas::api::utils::perf::Probe& graph_balance_probe
				) : 
			load_balancing_algorithm(lb_algorithm_probe, probed_lb),
			// The FPMAS LoadBalancingTask will call the
			// DistributedGraph::balance() method, that implicitly call this
			// class balance() methods, but also the distribution process,
			// implicitly feeding the probes 
			lb_task(graph, load_balancing_algorithm),
			graph_balance_probe(graph_balance_probe),
			// The job only contains this task
			job({*this}) {
		}

		/**
		 * Runs the LoadBalancingTask and probes it.
		 *
		 * The LoadBalancingTask calls the
		 * [DistributedGraph::balance()](https://fpmas.github.io/FPMAS/classfpmas_1_1api_1_1graph_1_1DistributedGraph.html#ab70f587553df3372f63b641c4c21944a) method,
		 * that includes load balancing **and** distribution.
		 */
		void run() override;

};

/**
 * fpmas::scheduler::Task that can be used to replace an
 * AgentGroup::agentExecutionJob() end task to probe the graph synchronization
 * process.
 */
class SyncProbeTask : public fpmas::scheduler::Task {
	private:
		fpmas::api::utils::perf::Probe& sync_probe;
		fpmas::model::detail::SynchronizeGraphTask sync_task;

	public:
		/**
		 * SyncProbeTask constructor.
		 *
		 * @param sync_probe Probe used to measure the synchronization time
		 * @param graph Distributed graph to synchronize
		 */
		SyncProbeTask(
				fpmas::api::utils::perf::Probe& sync_probe,
				fpmas::api::model::AgentGraph& graph);

		/**
		 * Calls the DistribtedGraph::synchronize() method and probes its
		 * execution time.
		 */
		void run() override;
};

