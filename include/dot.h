#include "fpmas/api/model/model.h"
#include "fpmas/io/output.h"

namespace dot {

	void dot_output(const fpmas::api::model::AgentGraph& graph);

	class NodeView {
		public:
			DistributedId id;
			int location;
			int x;
			int y;
			bool is_located;
		private:
			NodeView(DistributedId id, int location, bool is_located, int x, int y)
				: id(id), location(location), is_located(is_located), x(x), y(y) {
				}

		public:
			NodeView() = default;

			NodeView(DistributedId id, int location)
				: NodeView(id, location, false, {}, {}) {
				}
			NodeView(DistributedId id, int location, int x, int y)
				: NodeView(id, location, true, x, y) {
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
