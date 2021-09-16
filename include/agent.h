#include "fpmas.h"
#include "grid.h"

class BenchmarkAgent : public fpmas::model::GridAgent<BenchmarkAgent, BenchmarkCell> {
	public:
		fpmas::model::MooreRange<fpmas::model::MooreGrid<BenchmarkCell>> range;

		BenchmarkAgent() : range(1) {}

		FPMAS_MOBILITY_RANGE(range);
		FPMAS_PERCEPTION_RANGE(range);

		void move();
};

FPMAS_DEFAULT_JSON(BenchmarkAgent);
