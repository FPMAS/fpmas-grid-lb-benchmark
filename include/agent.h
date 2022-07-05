#pragma once

#include "fpmas.h"
#include "grid.h"

struct MovePolicyFunction {
	virtual MetaGridCell* selectCell(
			fpmas::model::Neighbors<MetaGridCell>& mobility_field) const = 0;
};

struct RandomMovePolicy : public MovePolicyFunction {
	MetaGridCell* selectCell(
			fpmas::model::Neighbors<MetaGridCell>& mobility_field) const override;
};

struct MaxMovePolicy : public MovePolicyFunction {
	MetaGridCell* selectCell(
			fpmas::model::Neighbors<MetaGridCell>& mobility_field) const override;
};

class MetaAgent : public GridAgent<MetaAgent, MetaGridCell> {
	public:
		static std::size_t max_contacts;
		static std::size_t range_size;
		static float contact_weight;
		static MovePolicy move_policy;

		std::deque<DistributedId> _contacts;

		/**
		 * Adds `agent` to the contact list (at the end of the queue) and links
		 * is as an outgoing neighbor of this agent on the CONTACT layer.
		 */
		void add_to_contacts(fpmas::api::model::Agent* agent);
		/**
		 * Checks if the agent corresponding to `id` is currently in the
		 * contacts list of this agent.
		 *
		 * This method can safely be called on a DISTANT agent after a _read_
		 * operation.
		 */
		bool is_in_contacts(DistributedId id);

	public:
		MooreRange<MooreGrid<MetaGridCell>> range;

		MetaAgent() : range(range_size) {}
		MetaAgent(const std::deque<DistributedId>& contacts)
			: _contacts(contacts), range(range_size) {}

		FPMAS_MOBILITY_RANGE(range);
		FPMAS_PERCEPTION_RANGE(range);

		const std::deque<DistributedId>& contacts() const;

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

		static void to_json(nlohmann::json& j, const MetaAgent* agent);
		static MetaAgent* from_json(const nlohmann::json& j);

		static std::size_t size(const fpmas::io::datapack::ObjectPack& o, const MetaAgent* agent);
		static void to_datapack(
				fpmas::io::datapack::ObjectPack& o, const MetaAgent* agent);
		static MetaAgent* from_datapack(const fpmas::io::datapack::ObjectPack& o);
};
