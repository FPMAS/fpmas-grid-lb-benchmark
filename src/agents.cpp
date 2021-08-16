#include "agents.h"

void BenchmarkAgent::move() {
	auto mobility_field = this->mobilityField();
	this->moveTo(mobility_field.random());
}
