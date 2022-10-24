#pragma once
#include "fpmas.h"
#include "fpmas/utils/perf.h"
#include <fpmas/api/graph/location_state.h>

/**
 * @file interactions.h
 * MetaAgent interactions related features.
 */

/**
 * Random generator used to select random agents in the ReaderWriter
 * interactions.
 */
extern fpmas::random::DistributedGenerator<> random_interactions;

/**
 * Generic implementation of read/write behaviors defined is Interactions.
 *
 * The read `TargetAgent` type could normally be any MetaAgent or MetaCell type,
 * but currently only read/write operations among MetaGraphCells or
 * MetaGridCells are supported by the MetaModel.
 */
struct ReaderWriter {
	/**
	 * Probe used to measure the time passed in
	 * [ReadGuards](https://fpmas.github.io/FPMAS/classfpmas_1_1model_1_1ReadGuard.html)
	 * for read operations between two LOCAL agents.
	 */
	static fpmas::utils::perf::Probe local_read_probe;
	/**
	 * Probe used to measure the time passed in
	 * [AcquireGuards](https://fpmas.github.io/FPMAS/classfpmas_1_1model_1_1AcquireGuard.html)
	 * for write operations between two LOCAL agents.
	 */
	static fpmas::utils::perf::Probe local_write_probe;
	/**
	 * Probe used to measure the time passed in
	 * [ReadGuards](https://fpmas.github.io/FPMAS/classfpmas_1_1model_1_1ReadGuard.html)
	 * for read operations between two DISTANT agents.
	 */
	static fpmas::utils::perf::Probe distant_read_probe;
	/**
	 * Probe used to measure the time passed in
	 * [AcquireGuards](https://fpmas.github.io/FPMAS/classfpmas_1_1model_1_1AcquireGuard.html)
	 * for write operations between two DISTANT agents.
	 */
	static fpmas::utils::perf::Probe distant_write_probe;

	/**
	 * Applies a `ReadGuard` on all neighbors.
	 *
	 * @param neighbors list of neighbors to interact with 
	 */
	static void read_all(
			const fpmas::model::Neighbors<fpmas::api::model::Agent>& neighbors);

	/**
	 * Applies an `AcquireGuard` on all neighbors.
	 *
	 * @param neighbors list of neighbors to interact with 
	 */
	static void write_all(
			fpmas::model::Neighbors<fpmas::api::model::Agent>& neighbors);

	/**
	 * Applies a `ReadGuard` on a randomly selected neighbor, using the
	 * #random_interactions generator.
	 *
	 * @param neighbors list of neighbors to interact with 
	 */
	static void read_one(
			const fpmas::model::Neighbors<fpmas::api::model::Agent>& neighbors);

	/**
	 * Applies an `AcquireGuard` on a randomly selected neighbor, using the
	 * #random_interactions generator.
	 *
	 * @param neighbors list of neighbors to interact with 
	 */
	static void write_one(
			fpmas::model::Neighbors<fpmas::api::model::Agent>& neighbors);

	/**
	 * Applies read_all(), then write_all().
	 *
	 * @param neighbors list of neighbors to interact with 
	 */
	static void read_all_write_all(
			fpmas::model::Neighbors<fpmas::api::model::Agent>& neighbors);

	/**
	 * Applies read_all(), then write_one().
	 *
	 * @param neighbors list of neighbors to interact with 
	 */
	static void read_all_write_one(
			fpmas::model::Neighbors<fpmas::api::model::Agent>& neighbors);
};

