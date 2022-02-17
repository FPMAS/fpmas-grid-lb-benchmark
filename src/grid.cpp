#include "grid.h"
#include "fpmas/api/model/spatial/spatial_model.h"

void BenchmarkCell::update_edge_weights() {
	std::size_t agent_count
		= this->node()->getIncomingEdges(fpmas::api::model::LOCATION).size();
	for(auto edge : this->node()->getOutgoingEdges(fpmas::api::model::CELL_SUCCESSOR))
		edge->setWeight(agent_count);
};

void BenchmarkCell::to_json(nlohmann::json &j, const BenchmarkCell *cell) {
	j = cell->utility;
}

BenchmarkCell* BenchmarkCell::from_json(const nlohmann::json& j) {
	return new BenchmarkCell(j.get<float>());
}

std::size_t BenchmarkCell::size(
		const fpmas::io::datapack::ObjectPack &o, const BenchmarkCell *cell) {
	return o.size<float>();
}

void BenchmarkCell::to_datapack(
		fpmas::io::datapack::ObjectPack &o, const BenchmarkCell *cell) {
	o.put(cell->getUtility());
}

BenchmarkCell* BenchmarkCell::from_datapack(const fpmas::io::datapack::ObjectPack& o) {
	return new BenchmarkCell(o.get<float>());
}

float UniformUtility::utility(Attractor, DiscretePoint) const {
	return 1.f;
}

float LinearUtility::utility(Attractor attractor, DiscretePoint point) const {
	return std::max(
			0.f, attractor.radius - fpmas::api::model::euclidian_distance(
				attractor.center, point
				)
			);
}

float InverseUtility::utility(Attractor attractor, DiscretePoint point) const {
	// 1/x like utility function depending on the distance from the center.
	// Utility=1 at center
	// Utility=beta when distance=radius
	float beta = 0.5;
	float alpha = (1 - beta) / (beta * attractor.radius);
	return 1 / (1 + alpha * (fpmas::api::model::euclidian_distance(
				attractor.center, point
				)-offset));
}

float StepUtility::utility(Attractor attractor, DiscretePoint point) const {
	if(fpmas::api::model::euclidian_distance(
			attractor.center, point
			) > attractor.radius)
		return InverseUtility(attractor.radius).utility(attractor, point);
	else
		return 1.f;
}

BenchmarkCell* BenchmarkCellFactory::build(fpmas::model::DiscretePoint location) {
	float utility = 0;
	for(auto attractor : attractors) {
		utility += utility_function.utility(attractor, location);
	}
	return new BenchmarkCell(location, utility);
}

