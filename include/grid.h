#pragma once

#include "fpmas.h"
#include "config.h"

using namespace fpmas::model;

class MetaGridCell : public GridCellBase<MetaGridCell> {
	private:
		float utility;

		// For JSON serialization
		MetaGridCell(float utility)
			: utility(utility) {
			}

	public:
		// For edge migration optimization purpose only
		MetaGridCell() {
		}

		// For cell factory
		MetaGridCell(DiscretePoint location, float utility)
			: GridCellBase<MetaGridCell>(location), utility(utility) {
			}

		void update_edge_weights();

		float getUtility() const {
			return utility;
		}

		static void to_json(nlohmann::json& j, const MetaGridCell* cell);
		static MetaGridCell* from_json(const nlohmann::json& j);

		static std::size_t size(const fpmas::io::datapack::ObjectPack& o, const MetaGridCell* cell);
		static void to_datapack(fpmas::io::datapack::ObjectPack& o, const MetaGridCell* cell);
		static MetaGridCell* from_datapack(const fpmas::io::datapack::ObjectPack& o);
};

struct UtilityFunction {
	virtual float utility(Attractor attractor, DiscretePoint point) const = 0;

	virtual ~UtilityFunction() {
	}
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


class MetaGridCellFactory : public fpmas::api::model::GridCellFactory<MetaGridCell> {
	public:
		const UtilityFunction& utility_function;
		std::vector<Attractor> attractors;

		MetaGridCellFactory(
				const UtilityFunction& utility_function,
				std::vector<Attractor> attractors
				) : utility_function(utility_function), attractors(attractors) {
		}

		MetaGridCell* build(DiscretePoint location) override;
};

