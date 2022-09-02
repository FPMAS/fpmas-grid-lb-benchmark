#pragma once
#include "fpmas.h"
#include "fpmas/utils/perf.h"
#include <fpmas/api/graph/location_state.h>

template<typename SourceAgent, typename TargetAgent>
struct ReaderWriter {
	static fpmas::utils::perf::Probe local_read_probe;
	static fpmas::utils::perf::Probe local_write_probe;
	static fpmas::utils::perf::Probe distant_read_probe;
	static fpmas::utils::perf::Probe distant_write_probe;

	static void read_all(
			SourceAgent* agent,
			const fpmas::model::Neighbors<TargetAgent>& neighbors) {
		for(auto& neighbor : neighbors) {
			fpmas::utils::perf::Probe& read_probe =
				neighbor->node()->state() == fpmas::api::graph::LOCAL ?
				local_read_probe : distant_read_probe;
			read_probe.start();
			{
				fpmas::model::ReadGuard read(neighbor);
			}
			read_probe.stop();
		}
	}
	static void write_all(
			SourceAgent* agent,
			fpmas::model::Neighbors<TargetAgent>& neighbors) {
		for(auto& neighbor : neighbors) {
			fpmas::utils::perf::Probe& write_probe =
				neighbor->node()->state() == fpmas::api::graph::LOCAL ?
				local_write_probe : distant_write_probe;
			write_probe.start();
			{
				fpmas::model::AcquireGuard acq(neighbor);
			}
			write_probe.stop();
		}
	}
	static void read_one(
			SourceAgent* agent,
			const fpmas::model::Neighbors<TargetAgent>& neighbors) {
		if(neighbors.count() > 0) {
			const TargetAgent* neighbor = neighbors.random(agent->rd());
			fpmas::utils::perf::Probe& read_probe =
				neighbor->node()->state() == fpmas::api::graph::LOCAL ?
				local_read_probe : distant_read_probe;
			read_probe.start();
			{
				fpmas::model::ReadGuard read(neighbor);
			}
			read_probe.stop();
		}
	}
	static void write_one(
			SourceAgent* agent,
			fpmas::model::Neighbors<TargetAgent>& neighbors) {
		if(neighbors.count() > 0) {
			TargetAgent* neighbor = neighbors.random(agent->rd());
			fpmas::utils::perf::Probe& write_probe =
				neighbor->node()->state() == fpmas::api::graph::LOCAL ?
				local_write_probe : distant_write_probe;
			write_probe.start();
			{
				fpmas::model::AcquireGuard acq(neighbor);
			}
			write_probe.stop();
		}
	}
	static void read_all_write_all(
			SourceAgent* agent,
			fpmas::model::Neighbors<TargetAgent>& neighbors) {
		read_all(agent, neighbors);
		write_all(agent, neighbors);
	}
	static void read_all_write_one(
			SourceAgent* agent,
			fpmas::model::Neighbors<TargetAgent>& neighbors) {
		read_all(agent, neighbors);
		write_one(agent, neighbors);
	}
};

template<typename SourceAgent, typename TargetAgent>
fpmas::utils::perf::Probe ReaderWriter<SourceAgent, TargetAgent>::local_read_probe {
	"LOCAL_READ"
};
template<typename SourceAgent, typename TargetAgent>
fpmas::utils::perf::Probe ReaderWriter<SourceAgent, TargetAgent>::distant_read_probe {
	"DISTANT_READ"
};

template<typename SourceAgent, typename TargetAgent>
fpmas::utils::perf::Probe ReaderWriter<SourceAgent, TargetAgent>::local_write_probe {
	"LOCAL_WRITE"
};
template<typename SourceAgent, typename TargetAgent>
fpmas::utils::perf::Probe ReaderWriter<SourceAgent, TargetAgent>::distant_write_probe {
	"DISTANT_WRITE"
};
