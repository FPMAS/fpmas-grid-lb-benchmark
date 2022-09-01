#pragma once

#include "config.h"
#include "interactions.h"

using namespace fpmas::model;

class MetaCell {
	private:
		float utility;

	public:
		// For edge migration optimization purpose only
		MetaCell() = default;

		// For JSON serialization
		MetaCell(float utility)
			: utility(utility) {
			}

		float getUtility() const {
			return utility;
		}

		virtual fpmas::api::model::AgentNode* node() = 0;
};

class MetaSpatialCell : public MetaCell {
	public:
		static float cell_edge_weight;

		void update_edge_weights();
		virtual void read_all_cell() = 0;
		virtual void read_one_cell() = 0;
		virtual void write_all_cell() = 0;
		virtual void write_one_cell() = 0;
		virtual void read_all_write_one_cell() = 0;
		virtual void read_all_write_all_cell() = 0;

		using MetaCell::MetaCell;
};

template<typename CellType>
struct CellSerialization {
	static void to_json(nlohmann::json &j, const CellType *cell) {
		j = cell->getUtility();
	}

	static CellType* from_json(const nlohmann::json& j) {
		return new CellType(j.get<float>());
	}

	static std::size_t size(
			const fpmas::io::datapack::ObjectPack &o, const CellType *cell) {
		return o.size<float>();
	}

	static void to_datapack(
			fpmas::io::datapack::ObjectPack &o, const CellType *cell) {
		o.put(cell->getUtility());
	}

	static CellType* from_datapack(const fpmas::io::datapack::ObjectPack& o) {
		float utility = o.get<float>();
		return new CellType(utility);
	}
};

#define IMPLEM_CELL_INTERACTION(INTERACTION, CELL_TYPE)\
	void INTERACTION##_cell() override {\
		auto neighbors = this->outNeighbors<CELL_TYPE>(fpmas::api::model::CELL_SUCCESSOR);\
		ReaderWriter<CELL_TYPE, CELL_TYPE>::INTERACTION(\
				this, neighbors\
				);\
	}

#define IMPLEM_CELL_INTERACTIONS(CELL_TYPE)\
	IMPLEM_CELL_INTERACTION(read_all, CELL_TYPE)\
	IMPLEM_CELL_INTERACTION(read_one, CELL_TYPE)\
	IMPLEM_CELL_INTERACTION(write_all, CELL_TYPE)\
	IMPLEM_CELL_INTERACTION(write_one, CELL_TYPE)\
	IMPLEM_CELL_INTERACTION(read_all_write_one, CELL_TYPE)\
	IMPLEM_CELL_INTERACTION(read_all_write_all, CELL_TYPE)

class MetaGridCell :
	public MetaSpatialCell,
	public GridCellBase<MetaGridCell>,
	public CellSerialization<MetaGridCell> {
	public:
		// For edge migration optimization purpose only
		using MetaSpatialCell::MetaSpatialCell;

		// For cell factory
		MetaGridCell(DiscretePoint location, float utility)
			: GridCellBase<MetaGridCell>(location), MetaSpatialCell(utility) {
			}

		fpmas::api::model::AgentNode* node() override {
			return this->GridCellBase<MetaGridCell>::node();
		}

		IMPLEM_CELL_INTERACTIONS(MetaGridCell);
		
};

class MetaGraphCell :
	public MetaSpatialCell,
	public GraphCellBase<MetaGraphCell>,
	public CellSerialization<MetaGraphCell> {
		private:
			static fpmas::random::DistributedGenerator<> _rd;
		public:
				// For edge migration optimization purpose only
				using MetaSpatialCell::MetaSpatialCell;

				// For cell factory
				MetaGraphCell(float utility)
					: GraphCellBase<MetaGraphCell>(), MetaSpatialCell(utility) {
					}

				fpmas::api::model::AgentNode* node() override {
					return this->GraphCellBase<MetaGraphCell>::node();
				}

				fpmas::random::DistributedGenerator<>& rd() {
					return _rd;
				}

				IMPLEM_CELL_INTERACTIONS(MetaGraphCell);
	};

struct UtilityFunction {
	virtual float utility(GridAttractor attractor, DiscretePoint point) const = 0;

	virtual ~UtilityFunction() {
	}
};

struct UniformUtility : public UtilityFunction {
	float utility(GridAttractor attractor, DiscretePoint point) const override;
};

struct LinearUtility : public UtilityFunction {
	float utility(GridAttractor attractor, DiscretePoint point) const override;
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

		float utility(GridAttractor attractor, DiscretePoint point) const override;
};

struct StepUtility : public UtilityFunction {
	float utility(GridAttractor attractor, DiscretePoint point) const override;
};


class MetaGridCellFactory : public fpmas::api::model::GridCellFactory<MetaGridCell> {
	public:
		const UtilityFunction& utility_function;
		std::vector<GridAttractor> attractors;

		MetaGridCellFactory(
				const UtilityFunction& utility_function,
				std::vector<GridAttractor> attractors
				) : utility_function(utility_function), attractors(attractors) {
		}

		MetaGridCell* build(DiscretePoint location) override;
};
