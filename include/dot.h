#pragma once

#include "fpmas/api/model/model.h"
#include "fpmas/io/output.h"

class BasicMetaModel;

class NodeView {
	public:
		DistributedId id;
		int rank;
		int x;
		int y;
		bool is_location;
		bool fixed_position;
		float utility;

	private:
		NodeView(
				DistributedId id, int rank, bool is_location,
				float utility, bool fixed_position, int x, int y) :
			id(id), rank(rank), is_location(is_location),
			utility(utility), fixed_position(fixed_position), x(x), y(y) {
			}

	public:
		NodeView() = default;

		NodeView(DistributedId id, int rank, float utility)
			: NodeView(id, rank, true, utility, false, {}, {}) {
			}
		NodeView(DistributedId id, int rank)
			: NodeView(id, rank, false, 1.0f, false, {}, {}) {
			}
		NodeView(DistributedId id, int rank, float utility, int x, int y)
			: NodeView(id, rank, true, utility, true, x, y) {
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

class DotOutput : public fpmas::io::OutputBase {
	private:
		fpmas::io::DynamicFileOutput output_file;
		BasicMetaModel& meta_model;

	public:
		DotOutput(
				BasicMetaModel& meta_model,
				std::string filename
				);

		void dump() override;
};
