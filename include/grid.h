#pragma once

#include "fpmas.h"
#include "config.h"

using namespace fpmas::model;

class BenchmarkCell : public GridCellBase<BenchmarkCell> {
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
		BenchmarkCell(DiscretePoint location, float utility)
			: GridCellBase<BenchmarkCell>(location), utility(utility) {
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

struct UtilityFunction {
	virtual float utility(Attractor attractor, DiscretePoint point) const = 0;
};

struct UniformUtility : public UtilityFunction {
	float utility(Attractor attractor, DiscretePoint point) const override;
};

struct LinearUtility : public UtilityFunction {
	float utility(Attractor attractor, DiscretePoint point) const override;
};

class InverseUtility : public UtilityFunction {
	private:
		float offset;

	public:
		/**
		 * InverseUtility constructor.
		 *
		 * Equivalent to `InverseUtility(0.f)`.
		 */
		InverseUtility() : InverseUtility(0.f) {
		}

		/**
		 * InverseUtility constructor.
		 *
		 * @param offset distance at which the utility value is 1.f
		 */
		InverseUtility(float offset) : offset(offset) {
		}

		float utility(Attractor attractor, DiscretePoint point) const override;
};

struct StepUtility : public UtilityFunction {
	float utility(Attractor attractor, DiscretePoint point) const override;
};


class BenchmarkCellFactory : public fpmas::api::model::GridCellFactory<BenchmarkCell> {
	public:
		const UtilityFunction& utility_function;
		std::vector<Attractor> attractors;

		BenchmarkCellFactory(
				const UtilityFunction& utility_function,
				std::vector<Attractor> attractors
				) : utility_function(utility_function), attractors(attractors) {
		}

		BenchmarkCell* build(DiscretePoint location) override;
};

