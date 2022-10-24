#include "interactions.h"

fpmas::random::DistributedGenerator<> random_interactions;

fpmas::utils::perf::Probe ReaderWriter::local_read_probe {
	"LOCAL_READ"
};
fpmas::utils::perf::Probe ReaderWriter::distant_read_probe {
	"DISTANT_READ"
};

fpmas::utils::perf::Probe ReaderWriter::local_write_probe {
	"LOCAL_WRITE"
};
fpmas::utils::perf::Probe ReaderWriter::distant_write_probe {
	"DISTANT_WRITE"
};

void ReaderWriter::read_all(
		const fpmas::model::Neighbors<fpmas::api::model::Agent>& neighbors) {
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
void ReaderWriter::write_all(
		fpmas::model::Neighbors<fpmas::api::model::Agent>& neighbors) {
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
void ReaderWriter::read_one(
		const fpmas::model::Neighbors<fpmas::api::model::Agent>& neighbors) {
	if(neighbors.count() > 0) {
		const fpmas::api::model::Agent* neighbor
			= neighbors.random(random_interactions);
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
void ReaderWriter::write_one(
		fpmas::model::Neighbors<fpmas::api::model::Agent>& neighbors) {
	if(neighbors.count() > 0) {
		fpmas::api::model::Agent* neighbor = neighbors.random(random_interactions);
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
void ReaderWriter::read_all_write_all(
		fpmas::model::Neighbors<fpmas::api::model::Agent>& neighbors) {
	read_all(neighbors);
	write_all(neighbors);
}
void ReaderWriter::read_all_write_one(
		fpmas::model::Neighbors<fpmas::api::model::Agent>& neighbors) {
	read_all(neighbors);
	write_one(neighbors);
}

