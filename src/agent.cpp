#include "agent.h"


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
