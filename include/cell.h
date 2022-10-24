#pragma once

#include "config.h"
#include "interactions.h"

using namespace fpmas::model;

/**
 * @file cell.h
 * Contains MetaCell features.
 */

/**
 * A generic MetaCell class, that defines features common for both
 * MetaGridCells and MetaGraphCells.
 */
class MetaCell {
	public:
		/**
		 * Default weight of CELL_SUCCESSOR edges.
		 */
		static float cell_edge_weight;

	private:
		float utility;
		std::vector<char> data;

	public:
		// For edge migration optimization purpose only

		/**
		 * Default generated MetaCell constructor, for internal use only.
		 */
		MetaCell() = default;

		// For JSON serialization

		/**
		 * MetaCell constructor.
		 *
		 * The vector of dummy data is allocated and default initialized with
		 * the specified count of elements.
		 *
		 * @param utility Utility of the cell
		 * @param cell_size Size of the vector of dummy data
		 */
		MetaCell(float utility, std::size_t cell_size)
			: MetaCell(utility, std::vector<char>(cell_size)) {
			}
		
		/**
		 * MetaCell constructor.
		 *
		 * @param utility Utility of the cell
		 * @param data Vector of dummy data
		 */
		MetaCell(float utility, const std::vector<char>& data)
			: utility(utility), data(data) {
			}

		/**
		 * Utility associated to this cell.
		 */
		float getUtility() const {
			return utility;
		}
		/**
		 * Dummy data used to emulate different serialisation sizes.
		 *
		 * @see ModelConfig::cell_size
		 */
		const std::vector<char>& getData() const {
			return data;
		}

		/**
		 * Sets the weight of outgoing CELL_SUCCESSOR edges to
		 * `cell_edge_weight+[count of agents located in the cell]`.
		 *
		 * This can be useful to better handle the DistributedMoveAlgorithm with
		 * graph based load balancing algorithms.
		 *
		 * @see ModelConfig::dynamic_cell_edge_weights
		 */
		void update_edge_weights();

		/**
		 * Interactions::READ_ALL behavior implementation.
		 */
		virtual void read_all_cell() = 0;
		/**
		 * Interactions::READ_ONE behavior implementation.
		 */
		virtual void read_one_cell() = 0;
		/**
		 * Interactions::WRITE_ALL behavior implementation.
		 */
		virtual void write_all_cell() = 0;
		/**
		 * Interactions::WRITE_ONE behavior implementation.
		 */
		virtual void write_one_cell() = 0;
		/**
		 * Interactions::READ_ALL_WRITE_ONE behavior implementation.
		 */
		virtual void read_all_write_one_cell() = 0;
		/**
		 * Interactions::READ_ALL_WRITE_ALL behavior implementation.
		 */
		virtual void read_all_write_all_cell() = 0;

		/**
		 * Returns a pointer to the node containing the cell.
		 */
		virtual const fpmas::api::model::AgentNode* cellNode() const = 0;
};

 /**
 * MetaCell JSON and ObjectPack serialization rules.
 *
 * The dummy MetaCell::getData() field is serialized, to produce a fake volume
 * of data.
 */
template<typename CellType>
struct CellSerialization {
	/**
	 * Json serialization.
	 */
	static void to_json(nlohmann::json &j, const CellType *cell) {
		j = {cell->getUtility(), cell->getData()};
	}

	/**
	 * Json deserialization.
	 */
	static CellType* from_json(const nlohmann::json& j) {
		return new CellType(j[0].get<float>(), j[1].get<std::vector<char>>());
	}

	/**
	 * ObjectPack size.
	 */
	static std::size_t size(
			const fpmas::io::datapack::ObjectPack &o, const CellType *cell) {
		return o.size<float>() + o.size(cell->getData());
	}

	/**
	 * ObjectPack serialization.
	 */
	static void to_datapack(
			fpmas::io::datapack::ObjectPack &o, const CellType *cell) {
		o.put(cell->getUtility());
		o.put(cell->getData());
	}

	/**
	 * ObjectPack deserialization.
	 */
	static CellType* from_datapack(const fpmas::io::datapack::ObjectPack& o) {
		float utility = o.get<float>();
		std::vector<char> data = o.get<std::vector<char>>();
		return new CellType(utility, data);
	}
};

#define IMPLEM_CELL_INTERACTION(INTERACTION, CELL_TYPE)\
	void INTERACTION##_cell() override {\
		auto neighbors = this->outNeighbors<fpmas::api::model::Agent>(fpmas::api::model::CELL_SUCCESSOR);\
		ReaderWriter::INTERACTION(neighbors);\
	}

#define IMPLEM_CELL_INTERACTIONS(CELL_TYPE)\
	IMPLEM_CELL_INTERACTION(read_all, CELL_TYPE)\
	IMPLEM_CELL_INTERACTION(read_one, CELL_TYPE)\
	IMPLEM_CELL_INTERACTION(write_all, CELL_TYPE)\
	IMPLEM_CELL_INTERACTION(write_one, CELL_TYPE)\
	IMPLEM_CELL_INTERACTION(read_all_write_one, CELL_TYPE)\
	IMPLEM_CELL_INTERACTION(read_all_write_all, CELL_TYPE)

/**
 * MetaCell extension of MetaGridModel.
 */
class MetaGridCell :
	public MetaCell,
	public GridCellBase<MetaGridCell>,
	public CellSerialization<MetaGridCell> {
	public:
		// For edge migration optimization purpose only
		using MetaCell::MetaCell;

		// For cell factory

		/**
		 * MetaGridCell constructor.
		 *
		 * @param location Discrete location of the cell
		 * @param utility Utility of the cell
		 * @param data Vector of dummy data
		 */
		MetaGridCell(DiscretePoint location, float utility, const std::vector<char>& data)
			: GridCellBase<MetaGridCell>(location), MetaCell(utility, data) {
			}
		/**
		 * MetaGridCell constructor.
		 *
		 * The vector of dummy data is allocated and default initialized with
		 * the specified count of elements.
		 *
		 * @param location Discrete location of the cell
		 * @param utility Utility of the cell
		 * @param cell_size Size of the vector of dummy data
		 */
		MetaGridCell(DiscretePoint location, float utility, std::size_t cell_size)
			: MetaGridCell(location, utility, std::vector<char>(cell_size)) {
			}

		const fpmas::api::model::AgentNode* cellNode() const override {
			return this->GridCellBase<MetaGridCell>::node();
		}

		IMPLEM_CELL_INTERACTIONS(MetaGridCell);
};

/**
 * MetaCell extension of MetaGraphModel.
 */
class MetaGraphCell :
	public MetaCell,
	public GraphCellBase<MetaGraphCell>,
	public CellSerialization<MetaGraphCell> {
		public:
			// For edge migration optimization purpose only
			using MetaCell::MetaCell;

			// For cell factory

			/**
			 * MetaGraphCell constructor.
			 *
			 * @param utility Utility of the cell
			 * @param data Vector of dummy data
			 */
			MetaGraphCell(float utility, const std::vector<char>& data)
				: GraphCellBase<MetaGraphCell>(), MetaCell(utility, data) {
				}

			/**
			 * MetaGraphCell constructor.
			 *
			 * The vector of dummy data is allocated and default initialized with
			 * the specified count of elements.
			 *
			 * @param utility Utility of the cell
			 * @param cell_size Size of the vector of dummy data
			 */
			MetaGraphCell(float utility, std::size_t cell_size)
				: MetaGraphCell(utility, std::vector<char>(cell_size)) {
				}

			const fpmas::api::model::AgentNode* cellNode() const override {
				return this->GraphCellBase<MetaGraphCell>::node();
			}

			IMPLEM_CELL_INTERACTIONS(MetaGraphCell);
	};

/**
 * Generic UtilityFunction interface.
 */
struct UtilityFunction {
	/**
	 * Returns an utility value associated to the specified point according to
	 * the given attractor.
	 *
	 * @param attractor Attractor from which the utility is computed
	 * @param point A discrete point of the grid environment
	 */
	virtual float utility(GridAttractor attractor, DiscretePoint point) const = 0;

	virtual ~UtilityFunction() {
	}
};

/**
 * Utility::UNIFORM implementation.
 *
 * The utility is the same for all cells.
 */
struct UniformUtility : public UtilityFunction {
	float utility(GridAttractor attractor, DiscretePoint point) const override;
};

/**
 * Utility::LINEAR implementation.
 *
 * The utility is set to 1 at the center of the attractor, decreases linearly to
 * 0 until the radius of the attractor, and is set to 0 after the radius.
 */
struct LinearUtility : public UtilityFunction {
	float utility(GridAttractor attractor, DiscretePoint point) const override;
};

/**
 * Utility::INVERSE implementation.
 *
 * The utility is set to 1 at the center of the attractor, and decreases to 0 at
 * infinity with a 1/x shape such as the utility value is 0.5 at the radius of
 * the attractor.
 */
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

/**
 * Utility::STEP implementation.
 *
 * The utility value is 1000 inside the radius of the attractor, and decreases
 * with an InverseUtility function after the radius, such as the utility is set
 * to 1 at the radius.
 */
struct StepUtility : public UtilityFunction {
	float utility(GridAttractor attractor, DiscretePoint point) const override;
};

/**
 * Factory class that can be used by a GridBuilder to build MetaGridCells.
 *
 * The factory is in charge of computing the initial utility of each cell.
 */
class MetaGridCellFactory : public fpmas::api::model::GridCellFactory<MetaGridCell> {
	private:
		const UtilityFunction& utility_function;
		std::vector<GridAttractor> attractors;
		std::size_t cell_size;

	public:
		/**
		 * MetaGridCellFactory constructor.
		 *
		 * The utility of each cell is computed as the sum of the utility
		 * generated by the utility_function for each attractor.
		 *
		 * @param utility_function Utility function
		 * @param attractors List of attractors used to generate utilities
		 * @param cell_size Dummy data size for each cell, in bytes
		 */
		MetaGridCellFactory(
				const UtilityFunction& utility_function,
				std::vector<GridAttractor> attractors,
				std::size_t cell_size) :
			utility_function(utility_function), attractors(attractors),
			cell_size(cell_size) {
		}

		/**
		 * Builds a MetaGridCell instance at the specified location with the
		 * specified cell_size.
		 */
		MetaGridCell* build(DiscretePoint location) override;
};

/**
 * Factory class that can be used by GraphBuilders to build MetaGraphCells.
 */
class MetaGraphCellFactory {
	private:
		std::size_t cell_size;
	public:
		/**
		 * MetaGraphCellFactory constructor.
		 *
		 * @param cell_size Dummy data size for each cell, in bytes
		 */
		MetaGraphCellFactory(std::size_t cell_size)
			: cell_size(cell_size) {
			}

		/**
		 * Builds a MetaGraphCell instance with the specified cell_size.
		 */
		MetaGraphCell* operator()();
};

