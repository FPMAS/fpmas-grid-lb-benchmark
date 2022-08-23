#include "agent.h"

std::size_t MetaAgentBase::max_contacts;
std::size_t MetaAgentBase::range_size = 1;
float MetaAgentBase::contact_weight = 1.0f;
MovePolicy MetaAgentBase::move_policy = MovePolicy::RANDOM;

std::deque<DistributedId>& MetaAgentBase::contacts() {
	return _contacts;
}

const std::deque<DistributedId>& MetaAgentBase::contacts() const {
	return _contacts;
}

bool MetaAgentBase::is_in_contacts(DistributedId id) {
	return std::find(_contacts.begin(), _contacts.end(), id) != _contacts.end();
}

