#include "agent.h"

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
	std::map<float, BenchmarkCell*> cells;
	std::vector<BenchmarkCell*> null_cells;
	float f = 0;
	for(auto cell : mobility_field) {
		fpmas::model::ReadGuard read(cell);
		if(cell->getUtility() > 0) {
			cells.insert({f, cell});
			f+= cell->getUtility();
		} else {
			null_cells.push_back(cell);
		}
	}

	BenchmarkCell* selected_cell;
	if(cells.empty()) {
		// Select in null_cells, that are ignored otherwise
		fpmas::random::UniformIntDistribution<std::size_t>
			rd_index(0, null_cells.size()-1);
		selected_cell = null_cells[rd_index(fpmas::model::RandomNeighbors::rd)];
	} else {
		fpmas::random::UniformRealDistribution<float> random_cell(0, f);
		// selects random number in (0:f] (instead of [0:f))
		float select_cell = f-random_cell(fpmas::model::RandomNeighbors::rd);

		auto upper_bound = cells.upper_bound(select_cell);

		// In this case, upper_bound is necessarily greater than
		// range.begin(), that corresponds to f=0, since select_cell > 0

		// Let's consider a set of {cell: utility}: {(a, x), (b, y)}, with x
		// and y greater than 0 (always the case in the cells map).
		// The cells dict is {0: a, x: b}, according to the cells definition
		// above, and f is selected in (0, x+y].
		// If f falls in (0, x), upper_bound corresponds to the
		// entry (x: b). Since x is actually the utility of a, we need to
		// select the entry before upper_bound, i.e. --upper_bound, and a is
		// selected.
		// If f falls in [x, x+y), range.second corresponds to cells.end(), we
		// take --upper_bound and b is selected.
		// As desired, the probability to select a is 1/x, and the probability
		// to select b is 1/(x+y - x) = 1/y.
		selected_cell=(--upper_bound)->second;
	}

	this->moveTo(selected_cell);
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
