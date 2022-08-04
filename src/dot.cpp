#include "dot.h"
#include "config.h"
#include <fpmas/communication/communication.h>
#include <fpmas/utils/macros.h>
#include <fpmas/utils/functional.h>
#include <fpmas/api/model/spatial/grid.h>

namespace dot {

	void to_json(nlohmann::json& json, const NodeView& node_view) {
		json[0] = node_view.id;
		json[1] = node_view.location;
		json[2] = node_view.is_located;
		if(node_view.is_located) {
			json[3] = node_view.x;
			json[4] = node_view.y;
		}
	}

	void from_json(const nlohmann::json& json, NodeView& node_view) {
		json[0].get_to(node_view.id);
		json[1].get_to(node_view.location);
		json[2].get_to(node_view.is_located);
		if(node_view.is_located) {
			json[3].get_to(node_view.x);
			json[4].get_to(node_view.y);
		}
	}

	void to_json(nlohmann::json& json, const EdgeView& edge_view) {
		json[0] = edge_view.id;
		json[1] = edge_view.src;
		json[2] = edge_view.tgt;
		json[3] = edge_view.layer;
	}

	void from_json(const nlohmann::json& json, EdgeView& edge_view) {
		json[0].get_to(edge_view.id);
		json[1].get_to(edge_view.src);
		json[2].get_to(edge_view.tgt);
		json[3].get_to(edge_view.layer);
	}

	void dot_output(const fpmas::api::model::AgentGraph& graph) {
		std::vector<NodeView> nodes;
		std::vector<EdgeView> edges;
		for(auto node : graph.getLocationManager().getLocalNodes()) {
			if(auto located_cell = dynamic_cast<fpmas::api::model::GridCell*>(node.second->data().get()))
				nodes.push_back({
						node.first, node.second->location(),
						(int) located_cell->location().x, (int) located_cell->location().y
						});
			else
				nodes.push_back({node.first, node.second->location()});
			for(auto edge : node.second->getOutgoingEdges())
				if(edge->getLayer() == fpmas::api::model::LOCATION ||
						edge->getLayer() == fpmas::api::model::PERCEPTION ||
						edge->getLayer() == fpmas::api::model::CELL_SUCCESSOR ||
						edge->getLayer() == CONTACT)
					edges.push_back({
							edge->getId(), node.first, edge->getTargetNode()->getId(),
							edge->getLayer()
							});
		}

		fpmas::communication::detail::TypedMpi<
			std::vector<NodeView>, fpmas::io::datapack::JsonPack>
			node_view_mpi(fpmas::communication::WORLD);
		nodes = fpmas::communication::reduce(
				node_view_mpi, 0, nodes, fpmas::utils::Concat());

		fpmas::communication::detail::TypedMpi<
			std::vector<EdgeView>, fpmas::io::datapack::JsonPack>
			edge_view_mpi(fpmas::communication::WORLD);
		edges = fpmas::communication::reduce(
				edge_view_mpi, 0, edges, fpmas::utils::Concat());

		FPMAS_ON_PROC(fpmas::communication::WORLD, 0) {
			fpmas::io::FileOutput file("graph.dot", graph.getMpiCommunicator().getRank());
			file.get() << "strict graph model {" << std::endl;
			file.get() << "overlap=true;size=\"10,10\";K=1;ratio=compress;outputorder=edgesfirst;" << std::endl;
			file.get() << "node [colorscheme=set19];" << std::endl;
			file.get() << "edge [colorscheme=set39,dir=none];" << std::endl;

			for(auto& node : nodes) {
				file.get() << "n" << node.id.rank() << "_" << node.id.id() << "["
					<< "fixedsize=true,label=\"\",";
				if(node.is_located)
					// Located nodes = cells
					file.get()
						<< "height=.3,width=.3,"

						<< "pos=\"" << node.x*2 << "," << node.y*2 << "!" << "\","
						<< "shape=diamond,"
						<< "style=filled,";
				else
					file.get()
						<< "height=.5,width=.5,"
						<< "style=filled,";
				file.get()
					<< "fillcolor=" << node.location+1 << ","
					<< "color=" << node.location+1
					<< "];" << std::endl;
			}
			for(auto& edge : edges) {
				file.get()
					<< "n" << edge.src.rank() << "_" << edge.src.id() << " -- "
					<< "n" << edge.tgt.rank() << "_" << edge.tgt.id() << " "
					<< "[color=" << 6+edge.layer << ",";
				if(
						edge.layer == fpmas::api::model::LOCATION ||
						edge.layer == fpmas::api::model::PERCEPTION ||
						edge.layer == CONTACT) {
					if(edge.layer == fpmas::api::model::LOCATION) {
						file.get() << "len=0.1,weight=10000,";
					}
					file.get()
						<< "style=bold,";
				} else {
					file.get()
						<< "style=solid,";
				}
				file.get() << "];" << std::endl;
			}

			file.get() << "}" << std::endl;
		}
	}
}
