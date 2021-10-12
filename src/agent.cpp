#include "agent.h"

const RandomMovePolicy RandomMovePolicy::instance;

BenchmarkCell* RandomMovePolicy::selectCell(fpmas::model::Neighbors<BenchmarkCell> &mobility_field) const {
	std::vector<float> utilities;
	std::vector<BenchmarkCell*> cells;

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
const MaxMovePolicy MaxMovePolicy::instance;

BenchmarkCell* MaxMovePolicy::selectCell(fpmas::model::Neighbors<BenchmarkCell> &mobility_field) const {
	// Prevents bias when several cells have the max value
	mobility_field.shuffle();

	std::vector<std::pair<BenchmarkCell*, float>> cells;
	for(auto cell : mobility_field) {
		fpmas::model::ReadGuard read(cell);
		cells.push_back({cell, cell->getUtility()});
	}
	return std::max_element(cells.begin(), cells.end(),
			[] (
				const std::pair<BenchmarkCell*, float>& a1,
				const std::pair<BenchmarkCell*, float>& a2
			   ) {
			return a1.second < a2.second;
			}
			)->first;
}
	
const MovePolicyFunction* BenchmarkAgent::move_policy;

void BenchmarkAgent::to_json(nlohmann::json& j, const BenchmarkAgent* agent) {
	j = agent->_contacts;
}

BenchmarkAgent* BenchmarkAgent::from_json(const nlohmann::json& j) {
	return new BenchmarkAgent(j.get<std::deque<DistributedId>>());
}

const std::deque<DistributedId>& BenchmarkAgent::contacts() const {
	return _contacts;
}

void BenchmarkAgent::move() {
	auto mobility_field = this->mobilityField();
	this->moveTo(move_policy->selectCell(mobility_field));
}

void BenchmarkAgent::add_to_contacts(fpmas::api::model::Agent* agent) {
	if(_contacts.size() == max_contacts) {
		for(auto edge : this->node()->getOutgoingEdges(CONTACT)) {
			// Finds the edge corresponding to the queue's head and unlinks it
			if(edge->getTargetNode()->getId() == _contacts.front()) {
				this->model()->graph().unlink(edge);
				// No need to loop until the last edge
				break;
			}
		}
		// Removes queue's head once unlinked
		_contacts.pop_front();
	}
	// Links the new contact...
	this->model()->link(this, agent, CONTACT);
	// ... and adds it at the end of the queue
	_contacts.push_back(agent->node()->getId());
}

bool BenchmarkAgent::is_in_contacts(DistributedId id) {
	return std::find(_contacts.begin(), _contacts.end(), id) != _contacts.end();
}

void BenchmarkAgent::create_relations_from_neighborhood() {
	// Agents currently in the Moore neighborhood
	auto perceptions = this->perceptions();
	// Shuffles perceptions, to ensure that a random perception will be
	// selected as a new contact
	perceptions.shuffle();

	auto current_perception = perceptions.begin();
	// Searches for an agent not already in this agent's contact in the current
	// Moore neighborhood
	while(current_perception != perceptions.end()) {
		if(!is_in_contacts(current_perception->agent()->node()->getId())) {
			add_to_contacts(*current_perception);

			// Ends while loop
			current_perception = perceptions.end();
		} else {
			current_perception++;
		}
	}
}

void BenchmarkAgent::create_relations_from_contacts() {
	auto contacts = this->outNeighbors<BenchmarkAgent>(CONTACT);
	if(contacts.count() >= 2) {
		// Selects a random contact and puts it at the begining of the list.
		std::size_t random_index =
			fpmas::random::UniformIntDistribution<std::size_t>(0, contacts.count()-1)(
					fpmas::model::RandomNeighbors::rd
					);
		std::swap(contacts[0], contacts[random_index]);
		// contacts[0] is now the contact to which we would like to add a new
		// contact from this agent's contacts.
		std::size_t i = 1;
		{
			// Reads contacts[0], so that is_in_contacts() can be called safely
			fpmas::model::ReadGuard read(contacts[0]);
			// Searches a contact from this agent's contacts that is not
			// already in contacts[0]'s contacts
			while(
					i < contacts.count() &&
					contacts[0]->is_in_contacts(contacts[i]->node()->getId()))
				i++;
		}
		if(i < contacts.count())
			// If founds, creates a NEW_CONTACT, that will be handled from
			// contacts[0] by handle_new_contacts() after the next
			// synchronization.
			this->model()->link(contacts[0], contacts[i], NEW_CONTACT);
	}
}

void BenchmarkAgent::handle_new_contacts() {
	auto new_contacts = this->outNeighbors<BenchmarkAgent>(NEW_CONTACT);
	for(auto new_contact : new_contacts) {
		// Adds new_contact to this agent's contacts
		add_to_contacts(new_contact);

		// Unlinks temporary NEW_CONTACT edge
		this->model()->unlink(new_contact.edge());
	}
}
