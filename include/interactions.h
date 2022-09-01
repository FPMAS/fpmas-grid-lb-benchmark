#pragma once
#include "fpmas.h"
#include "fpmas/utils/perf.h"

template<typename SourceAgent, typename TargetAgent>
struct ReaderWriter {
	static fpmas::utils::perf::Probe read_probe;
	static fpmas::utils::perf::Probe write_probe;

	static void read_all(
			SourceAgent* agent,
			const fpmas::model::Neighbors<TargetAgent>& neighbors) {
		for(auto& neighbor : neighbors) {
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
fpmas::utils::perf::Probe ReaderWriter<SourceAgent, TargetAgent>::read_probe {"READ"};

template<typename SourceAgent, typename TargetAgent>
fpmas::utils::perf::Probe ReaderWriter<SourceAgent, TargetAgent>::write_probe {"WRITE"};
