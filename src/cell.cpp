#include "cell.h"
#include "fpmas/api/model/spatial/spatial_model.h"

float MetaSpatialCell::cell_edge_weight = 1.0f;

void MetaSpatialCell::update_edge_weights() {
	std::size_t agent_count
		= this->node()->getIncomingEdges(fpmas::api::model::LOCATION).size();
	for(auto edge : this->node()->getOutgoingEdges(fpmas::api::model::CELL_SUCCESSOR))
		edge->setWeight(cell_edge_weight + agent_count);
};

fpmas::random::DistributedGenerator<> MetaGraphCell::_rd;

float UniformUtility::utility(GridAttractor, DiscretePoint) const {
	return 1.f;
}

float LinearUtility::utility(GridAttractor attractor, DiscretePoint point) const {
	return std::max(
			0.f, 1.0f - fpmas::api::model::euclidian_distance(
				attractor.center, point
				) / attractor.radius
			);
}

float InverseUtility::utility(GridAttractor attractor, DiscretePoint point) const {
	// 1/x like utility function depending on the distance from the center.
	// Utility=1 at center
	// Utility=beta when distance=radius
	float beta = 0.5;
	float alpha = (1 - beta) / (beta * attractor.radius);
	return 1 / (1 + alpha * (fpmas::api::model::euclidian_distance(
				attractor.center, point
				)-offset));
}

float StepUtility::utility(GridAttractor attractor, DiscretePoint point) const {
	if(fpmas::api::model::euclidian_distance(
			attractor.center, point
			) > attractor.radius)
		return InverseUtility(attractor.radius).utility(attractor, point);
	else
		return 1.f;
}

MetaGridCell* MetaGridCellFactory::build(fpmas::model::DiscretePoint location) {
	float utility = 0;
	for(auto attractor : attractors) {
		utility += utility_function.utility(attractor, location);
	}
	return new MetaGridCell(location, utility, cell_size);
}

MetaGraphCell* MetaGraphCellFactory::operator()() {
	return new MetaGraphCell(1.0f, cell_size);
}


