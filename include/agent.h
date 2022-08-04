#pragma once

#include "fpmas.h"
#include "cell.h"

template<typename CellType>
struct MovePolicyFunction {
	virtual CellType* selectCell(
			fpmas::model::Neighbors<CellType>& mobility_field) const = 0;
};

template<typename CellType>
struct RandomMovePolicy : public MovePolicyFunction<CellType> {
	CellType* selectCell(
			fpmas::model::Neighbors<CellType>& mobility_field) const override;
};

template<typename CellType>
struct MaxMovePolicy : public MovePolicyFunction<CellType> {
	CellType* selectCell(
			fpmas::model::Neighbors<CellType>& mobility_field) const override;
};

template<typename CellType>
CellType* RandomMovePolicy<CellType>::selectCell(
		fpmas::model::Neighbors<CellType> &mobility_field) const {
	std::vector<float> utilities;
	std::vector<CellType*> cells;

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

template<typename CellType>
CellType* MaxMovePolicy<CellType>::selectCell(
		fpmas::model::Neighbors<CellType> &mobility_field) const {
	// Prevents bias when several cells have the max value
	mobility_field.shuffle();

	std::vector<std::pair<CellType*, float>> cells;
	for(auto cell : mobility_field) {
		fpmas::model::ReadGuard read(cell);
		cells.push_back({cell, cell->getUtility()});
	}
	return std::max_element(cells.begin(), cells.end(),
			[] (
				const std::pair<CellType*, float>& a1,
				const std::pair<CellType*, float>& a2
			   ) {
			return a1.second < a2.second;
			}
			)->first;
}

class MetaAgentBase {
	public:
		static std::size_t max_contacts;
		static std::size_t range_size;
		static float contact_weight;
		static MovePolicy move_policy;
	private:
		std::deque<DistributedId> _contacts;
	protected:
		std::deque<DistributedId>& contacts();

	public:
		/**
		 * Checks if the agent corresponding to `id` is currently in the
		 * contacts list of this agent.
		 *
		 * This method can safely be called on a DISTANT agent after a _read_
		 * operation.
		 */
		bool is_in_contacts(DistributedId id);

		virtual const fpmas::api::model::AgentNode* node() const = 0;
	public:
		MetaAgentBase() {}
		MetaAgentBase(const std::deque<DistributedId>& contacts)
			: _contacts(contacts) {}

		const std::deque<DistributedId>& contacts() const;
};

template<typename AgentBase, typename PerceptionRange>
class MetaAgent : public AgentBase, public MetaAgentBase {
	private:
		PerceptionRange range;

		/**
		 * Adds `agent` to the contact list (at the end of the queue) and links
		 * in as an outgoing neighbor of this agent on the CONTACT layer.
		 */
		void add_to_contacts(fpmas::api::model::Agent* agent);

	public:
		MetaAgent() : range(range_size) {}
		MetaAgent(const std::deque<DistributedId>& contacts)
			: MetaAgentBase(contacts), range(range_size) {}

		FPMAS_MOBILITY_RANGE(range);
		FPMAS_PERCEPTION_RANGE(range);

		/**
		 * Pick an random agent in the current Moore neighborhood and adds it
		 * to the contact list.
		 *
		 * If the list was full, the oldest contact is removed.
		 */
		void create_relations_from_neighborhood();
		/**
		 * Puts two random contacts in relation.
		 *
		 * First, a random contact A is selected. Then, attempts to find a
		 * random contact B that is not already a contact of A.
		 *
		 * If such a contact is found, a link from A to B is created on the
		 * NEW_CONTACT layer. Such links must then be handled from A thanks to
		 * the handle_new_contacts() method, after a synchronization
		 * has been performed (to ensure links migration).
		 */
		void create_relations_from_contacts();
		/**
		 * Adds new contacts to the contact list from the NEW_CONTACT layer.
		 *
		 * If the contact list is full, oldest contacts are removed to make
		 * enough space for new contacts.
		 */
		void handle_new_contacts();
		/**
		 * Moves to a random cell in the Moore neighborhood.
		 *
		 * The probability for each cell to be selected is proportional to its
		 * _utility_.
		 */
		void move();

		const fpmas::api::model::AgentNode* node() const override {
			return this->AgentBase::node();
		}
};

template<typename AgentBase, typename PerceptionRange>
void MetaAgent<AgentBase, PerceptionRange>::add_to_contacts(fpmas::api::model::Agent* agent) {
	if(contacts().size() == max_contacts) {
		for(auto edge : this->node()->getOutgoingEdges(CONTACT)) {
			// Finds the edge corresponding to the queue's head and unlinks it
			if(edge->getTargetNode()->getId() == contacts().front()) {
				this->model()->graph().unlink(edge);
				// No need to loop until the last edge
				break;
			}
		}
		// Removes queue's head once unlinked
		contacts().pop_front();
	}
	// Links the new contact...
	this->model()->link(this, agent, CONTACT)->setWeight(MetaAgentBase::contact_weight);
	// ... and adds it at the end of the queue
	contacts().push_back(agent->node()->getId());
}

template<typename AgentBase, typename PerceptionRange>
void MetaAgent<AgentBase, PerceptionRange>::create_relations_from_neighborhood() {
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

template<typename AgentBase, typename PerceptionRange>
void MetaAgent<AgentBase, PerceptionRange>::create_relations_from_contacts() {
	auto contacts =
		this->template outNeighbors<MetaAgent<AgentBase, PerceptionRange>>(CONTACT);
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

template<typename AgentBase, typename PerceptionRange>
void MetaAgent<AgentBase, PerceptionRange>::handle_new_contacts() {
	auto new_contacts =
		this->template outNeighbors<MetaAgent<AgentBase, PerceptionRange>>(NEW_CONTACT);
	for(auto new_contact : new_contacts) {
		// Adds new_contact to this agent's contacts
		add_to_contacts(new_contact);

		// Unlinks temporary NEW_CONTACT edge
		this->model()->unlink(new_contact.edge());
	}
}

template<typename AgentBase, typename PerceptionRange>
void MetaAgent<AgentBase, PerceptionRange>::move() {
	auto mobility_field = this->mobilityField();
	typename AgentBase::Cell* selected_cell;
	switch(move_policy) {
		case RANDOM:
			selected_cell = RandomMovePolicy<typename AgentBase::Cell>()
				.selectCell(mobility_field);
			break;
		case MAX:
			selected_cell = MaxMovePolicy<typename AgentBase::Cell>()
				.selectCell(mobility_field);
			break;
	};

	this->moveTo(selected_cell);
}

template<typename AgentType>
struct MetaAgentSerialization {
		static void to_json(nlohmann::json& j, const AgentType* agent);
		static AgentType* from_json(const nlohmann::json& j);

		static std::size_t size(const fpmas::io::datapack::ObjectPack& o, const AgentType* agent);
		static void to_datapack(
				fpmas::io::datapack::ObjectPack& o, const AgentType* agent);
		static AgentType* from_datapack(const fpmas::io::datapack::ObjectPack& o);
};

template<typename AgentType>
void MetaAgentSerialization<AgentType>::to_json(nlohmann::json& j, const AgentType* agent) {
	j = agent->contacts();
}

template<typename AgentType>
AgentType* MetaAgentSerialization<AgentType>::from_json(const nlohmann::json& j) {
	return new AgentType(j.get<std::deque<DistributedId>>());
}

template<typename AgentType>
std::size_t MetaAgentSerialization<AgentType>::size(
		const fpmas::io::datapack::ObjectPack &o, const AgentType *agent) {
	return o.size(agent->contacts());
}

template<typename AgentType>
void MetaAgentSerialization<AgentType>::to_datapack(
		fpmas::io::datapack::ObjectPack& o, const AgentType* agent) {
	o.put(agent->contacts());
}

template<typename AgentType>
AgentType* MetaAgentSerialization<AgentType>::from_datapack(
		const fpmas::io::datapack::ObjectPack &o) {
	return new AgentType(o.get<std::deque<DistributedId>>());
}

class MetaGridAgent :
	public MetaAgent<
		GridAgent<MetaGridAgent, MetaGridCell>,
		MooreRange<MooreGrid<MetaGridCell>>>,
	public MetaAgentSerialization<MetaGridAgent> {
		public:
			using MetaAgent<
				GridAgent<MetaGridAgent, MetaGridCell>,
				MooreRange<MooreGrid<MetaGridCell>>>::MetaAgent;
};

class MetaGraphAgent :
	public MetaAgent<
		SpatialAgent<MetaGraphAgent, MetaGraphCell>, GraphRange<MetaGraphCell>
	> {
		using MetaAgent<
			SpatialAgent<MetaGraphAgent, MetaGraphCell>, GraphRange<MetaGraphCell>
			>::MetaAgent;
	};
