#include "agents.h"

BenchmarkCell* ClusteredBenchmarkCellFactory::build(fpmas::model::DiscretePoint location) {
	float utility = 0;
	for(auto attractor : attractors) {
		// 1/x like utility function depending on the distance from the center.
		// Utility=1 at center
		// Utility=0.1 when distance=radius
		float alpha = 0.99 / (0.1 * attractor.radius);
		utility += 1 / (1 + alpha * fpmas::api::model::euclidian_distance(attractor.center, location));
	}
	return new BenchmarkCell(location, utility);
}

void BenchmarkAgent::move() {
	auto mobility_field = this->mobilityField();
	std::map<float, BenchmarkCell*> cells;
	float f = 0;
	for(auto cell : mobility_field) {
		fpmas::model::ReadGuard read(cell);
		f+= cell->getUtility();
		cells[f] = cell;
	}
	fpmas::random::UniformRealDistribution<float> random_cell(0, f);
	float select_cell = random_cell(fpmas::model::RandomNeighbors::rd);
	BenchmarkCell* selected_cell = (*cells.upper_bound(select_cell)--).second;

	this->moveTo(selected_cell);
}
