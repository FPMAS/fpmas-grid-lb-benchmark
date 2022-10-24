#pragma once

#include "fpmas/api/model/model.h"
#include "fpmas/io/output.h"
#include <nlohmann/adl_serializer.hpp>

class BasicMetaModel;

/**
 * @file dot.h
 * Contains features used to perform a MetaModel DOT output.
 */

/**
 * Helper class to serialize nodes information for the DotOutput.
 */
class NodeView {
	public:
		/**
		 * ID of the node.
		 */
		DistributedId id;
		/**
		 * Current location of the node.
		 */
		int rank;
		/**
		 * If fixed_position is true, represents the x coordinate of the DOT
		 * node.
		 */
		int x;
		/**
		 * If fixed_position is true, represents the y coordinate of the DOT
		 * node.
		 */
		int y;
		/**
		 * True iff the node contains a Cell (MetaGraphCell or MetaGridCell).
		 */
		bool is_location;
		/**
		 * If true, the position of the node is set to be fixed in the DOT
		 * output.
		 *
		 * @see https://graphviz.org/docs/attrs/pos/
		 * @see https://graphviz.org/docs/attr-types/point/
		 */
		bool fixed_position;
		/**
		 * If the node contains a MetaCell, utility of that MetaCell.
		 */
		float utility;

	private:
		// Generic constructor
		NodeView(
				DistributedId id, int rank, bool is_location,
				float utility, bool fixed_position, int x, int y) :
			id(id), rank(rank), is_location(is_location),
			utility(utility), fixed_position(fixed_position), x(x), y(y) {
			}

	public:
		/**
		 * Default generated contructor, for internal use.
		 */
		NodeView() = default;

		/**
		 * NodeView constructor for a node containing a generic MetaCell.
		 *
		 * #fixed_position is set to false, and #x and #y are left default
		 * initialized.
		 *
		 * @param id ID of the MetaCell
		 * @param rank Current location of the MetaCell
		 * @param utility Current utility of the MetaCell
		 */
		NodeView(DistributedId id, int rank, float utility)
			: NodeView(id, rank, true, utility, false, {}, {}) {
			}
		/**
		 * NodeView constructor for a generic node.
		 *
		 * #fixed_position and #is_location are set to false, and #x, #y and
		 * #utility are left default initialized.
		 *
		 * @param id ID of the MetaCell
		 * @param rank Current location of the MetaCell
		 */
		NodeView(DistributedId id, int rank)
			: NodeView(id, rank, false, 1.0f, false, {}, {}) {
			}

		/**
		 * NodeView constructor for a MetaGridCell.
		 *
		 * #is_location and #fixed_position are set to true.
		 *
		 * @param id ID of the MetaCell
		 * @param rank Current location of the MetaCell
		 * @param utility Current utility of the MetaCell
		 * @param x Discrete x coordinate of the GridCell
		 * @param y Discrete y coordinate of the GridCell
		 */
		NodeView(DistributedId id, int rank, float utility, int x, int y)
			: NodeView(id, rank, true, utility, true, x, y) {
			}
};

/**
 * Helper class to serialize edges information for the DotOutput.
 */
struct EdgeView {
	/**
	 * ID of the edge.
	 */
	DistributedId id;
	/**
	 * ID of the source node.
	 */
	DistributedId src;
	/**
	 * ID of the target node.
	 */
	DistributedId tgt;
	/**
	 * Layer id of the edge.
	 */
	fpmas::api::graph::LayerId layer;

	/**
	 * Default generated contructor, for internal use.
	 */
	EdgeView() = default;
	/**
	 * EdgeView constructor.
	 *
	 * @param id ID of the edge
	 * @param src ID of the source node
	 * @param tgt ID of the target node
	 * @param layer Layer id of the edge.
	 */
	EdgeView(
			DistributedId id, DistributedId src, DistributedId tgt,
			fpmas::api::graph::LayerId layer)
		: id(id), src(src), tgt(tgt), layer(layer) {
		}
};

namespace nlohmann {
	/**
	 * NodeView JSON serialization rules.
	 */
	template<>
		struct adl_serializer<NodeView> {
			/**
			 * JSON serialization.
			 */
			static void to_json(nlohmann::json& json, const NodeView& node_view);
			/**
			 * JSON deserialization.
			 */
			static void from_json(const nlohmann::json& json, NodeView& node_view);
		};

	/**
	 * EdgeView JSON serialization rules.
	 */
	template<>
		struct adl_serializer<EdgeView> {
			/**
			 * JSON serialization.
			 */
			static void to_json(nlohmann::json& json, const EdgeView& node_view);
			/**
			 * JSON deserialization.
			 */
			static void from_json(const nlohmann::json& json, EdgeView& edge_view);
		};
}

/**
 * An output class that can be used to dump a representation of a MetaModel as a
 * [DOT file](https://graphviz.org/doc/info/lang.html).
 *
 * Classical [DOT layout engines](https://graphviz.org/docs/layouts/),
 * especially `neato`, `fdp` and `sfdp`, can be used externally to generate
 * beautiful representations of MetaModels.
 */
class DotOutput : public fpmas::io::OutputBase {
	private:
		fpmas::io::DynamicFileOutput output_file;
		BasicMetaModel& meta_model;

	public:
		/**
		 * DotOutput constructor.
		 *
		 * @param meta_model MetaModel to dump to a DOT file
		 * @param filename Name of the DOT file
		 */
		DotOutput(
				BasicMetaModel& meta_model,
				std::string filename
				);

		/**
		 * Dumps the MetaModel to the DOT file.
		 *
		 * Must be called on **all** processes.
		 */
		void dump() override;
};
