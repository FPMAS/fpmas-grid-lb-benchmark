#include "fpmas.h"

class BenchmarkAgent : public fpmas::model::GridAgent<BenchmarkAgent> {
	public:
		fpmas::model::MooreRange<fpmas::model::MooreGrid<>> range;

		BenchmarkAgent() : range(1) {}

		FPMAS_MOBILITY_RANGE(range);
		FPMAS_PERCEPTION_RANGE(range);

		void move();
};

FPMAS_DEFAULT_JSON(BenchmarkAgent);
