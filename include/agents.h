#include "fpmas.h"
#include "config.h"

class BenchmarkCell : public fpmas::model::GridCellBase<BenchmarkCell> {
	private:
		float utility;

		// For JSON serialization
		BenchmarkCell(float utility)
			: utility(utility) {
			}

	public:
		// For edge migration optimization purpose only
		BenchmarkCell() {
		}

		// For cell factory
		BenchmarkCell(fpmas::model::DiscretePoint location, float utility)
			: fpmas::model::GridCellBase<BenchmarkCell>(location), utility(utility) {
			}

		float getUtility() const {
			return utility;
		}

		static void to_json(nlohmann::json& j, const BenchmarkCell* cell) {
			j = cell->utility;
		}

		static BenchmarkCell* from_json(const nlohmann::json& j) {
			return new BenchmarkCell(j.get<float>());
		}
};

class UniformBenchmarkCellFactory : public fpmas::api::model::GridCellFactory<BenchmarkCell> {
	public:
		BenchmarkCell* build(fpmas::model::DiscretePoint location) override {
			return new BenchmarkCell(location, 1);
		}
};

class ClusteredBenchmarkCellFactory : public fpmas::api::model::GridCellFactory<BenchmarkCell> {
	public:
		std::vector<Attractor> attractors;

		ClusteredBenchmarkCellFactory(
				std::vector<Attractor> attractors
				) : attractors(attractors) {
		}

		BenchmarkCell* build(fpmas::model::DiscretePoint location) override;
};

class BenchmarkAgent : public fpmas::model::GridAgent<BenchmarkAgent, BenchmarkCell> {
	public:
		fpmas::model::MooreRange<fpmas::model::MooreGrid<BenchmarkCell>> range;

		BenchmarkAgent() : range(1) {}

		FPMAS_MOBILITY_RANGE(range);
		FPMAS_PERCEPTION_RANGE(range);

		void move();
};

FPMAS_DEFAULT_JSON(BenchmarkAgent);
