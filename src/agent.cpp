#include "agent.h"

std::size_t MetaAgentBase::max_contacts;
std::size_t MetaAgentBase::range_size;
float MetaAgentBase::contact_weight;
MovePolicy MetaAgentBase::move_policy;

std::deque<DistributedId>& MetaAgentBase::contacts() {
	return _contacts;
}

const std::deque<DistributedId>& MetaAgentBase::contacts() const {
	return _contacts;
}

bool MetaAgentBase::is_in_contacts(DistributedId id) {
	return std::find(_contacts.begin(), _contacts.end(), id) != _contacts.end();
}


MetaGridCell* RandomMovePolicy::selectCell(fpmas::model::Neighbors<MetaGridCell> &mobility_field) const {
	std::vector<float> utilities;
	std::vector<MetaGridCell*> cells;

	bool null_utilities = true;
	for(auto cell : mobility_field) {
		fpmas::model::ReadGuard read(cell);
		if(cell->getUtility() > 0)
			null_utilities = false;
		utilities.push_back(cell->getUtility());
		cells.push_back(cell);
	}
	std::size_t rd_index;
	if(null_utilities) {
		fpmas::random::UniformIntDistribution<std::size_t> rd_cell(0, cells.size()-1);
		rd_index = rd_cell(fpmas::model::RandomNeighbors::rd);
	} else {
		fpmas::random::DiscreteDistribution<std::size_t> rd_cell(utilities);
		rd_index = rd_cell(fpmas::model::RandomNeighbors::rd);
	}
	return cells[rd_index];
}

MetaGridCell* MaxMovePolicy::selectCell(fpmas::model::Neighbors<MetaGridCell> &mobility_field) const {
	// Prevents bias when several cells have the max value
	mobility_field.shuffle();

	std::vector<std::pair<MetaGridCell*, float>> cells;
	for(auto cell : mobility_field) {
		fpmas::model::ReadGuard read(cell);
		cells.push_back({cell, cell->getUtility()});
	}
	return std::max_element(cells.begin(), cells.end(),
			[] (
				const std::pair<MetaGridCell*, float>& a1,
				const std::pair<MetaGridCell*, float>& a2
			   ) {
			return a1.second < a2.second;
			}
			)->first;
}

