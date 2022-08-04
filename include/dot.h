#include "fpmas/api/model/model.h"
#include "fpmas/io/output.h"
#include "metamodel.h"

namespace dot {

	void dot_output(BasicMetaModel& meta_model);

	class NodeView {
		public:
			DistributedId id;
			int rank;
			int x;
			int y;
			bool is_location;
			bool fixed_position;
		private:
			NodeView(
					DistributedId id, int rank, bool is_location,
					bool fixed_position, int x, int y) :
				id(id), rank(rank), is_location(is_location),
				fixed_position(fixed_position), x(x), y(y) {
				}

		public:
			NodeView() = default;

			NodeView(DistributedId id, int rank, bool is_location)
				: NodeView(id, rank, is_location, false, {}, {}) {
				}
			NodeView(DistributedId id, int rank, bool is_location, int x, int y)
				: NodeView(id, rank, is_location, true, x, y) {
				}
	};

	struct EdgeView {
		DistributedId id;
		DistributedId src;
		DistributedId tgt;
		fpmas::api::graph::LayerId layer;

		EdgeView() = default;
		EdgeView(
				DistributedId id, DistributedId src, DistributedId tgt,
				fpmas::api::graph::LayerId layer)
			: id(id), src(src), tgt(tgt), layer(layer) {
			}
	};

	void to_json(nlohmann::json& json, const NodeView& node_view);
	void from_json(const nlohmann::json& json, NodeView& node_view);

	void to_json(nlohmann::json& json, const EdgeView& edge_view);
	void from_json(const nlohmann::json& json, EdgeView& edge_view);
}
