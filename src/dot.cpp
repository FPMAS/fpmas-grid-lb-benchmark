#include "dot.h"
#include "metamodel.h"
#include <fpmas/communication/communication.h>
#include <fpmas/utils/macros.h>
#include <fpmas/utils/functional.h>
#include <fpmas/api/model/spatial/grid.h>

DotOutput::DotOutput(
		BasicMetaModel& meta_model,
		std::string filename
		) :
	fpmas::io::OutputBase(output_file),
	output_file(
			filename + ".dot",
			meta_model.getModel().getMpiCommunicator(),
			meta_model.getModel().runtime()
			),
	meta_model(meta_model) {
	}

void DotOutput::dump() {
	std::vector<NodeView> nodes;
	std::vector<EdgeView> edges;

	for(auto& cell : meta_model.cellGroup().localAgents()) {
		// GridCells have a fixed location
		if(auto located_cell = dynamic_cast<fpmas::api::model::GridCell*>(cell->node()->data().get())) {
			nodes.push_back({
					cell->node()->getId(), cell->node()->location(), true,
					(int) located_cell->location().x, (int) located_cell->location().y
					});
			// Other cell types have not
		} else {
			nodes.push_back({
					cell->node()->getId(), cell->node()->location(), true,
					});
		}
		for(auto& edge : cell->node()->getOutgoingEdges(fpmas::api::model::CELL_SUCCESSOR)) {
			edges.push_back({
					edge->getId(),
					cell->node()->getId(), edge->getTargetNode()->getId(),
					edge->getLayer()
					});
		}
	}
	for(auto& agent : meta_model.agentGroup().localAgents()) {
		nodes.push_back({
				agent->node()->getId(), agent->node()->location(), false
				});
		std::vector<fpmas::model::AgentEdge*> agent_edges;
		auto contacts = agent->node()->getOutgoingEdges(CONTACT);
		auto perceptions = agent->node()->getOutgoingEdges(
				fpmas::api::model::PERCEPTION
				);
		auto location_edge = agent->node()->getOutgoingEdges(
				fpmas::api::model::LOCATION
				);
		agent_edges.insert(agent_edges.end(), contacts.begin(), contacts.end());
		agent_edges.insert(agent_edges.end(), perceptions.begin(), perceptions.end());
		agent_edges.insert(agent_edges.end(), location_edge.begin(), location_edge.end());
		for(auto& edge : agent_edges) {
			edges.push_back({
					edge->getId(),
					agent->node()->getId(), edge->getTargetNode()->getId(),
					CONTACT
					});
		}
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

	auto& file = output_file.get();
	FPMAS_ON_PROC(fpmas::communication::WORLD, 0) {
		file << "digraph model {" << std::endl;
		file << "overlap=true;size=\"10,10\";K=1;ratio=compress;outputorder=edgesfirst;" << std::endl;
		file << "node [colorscheme=set19];" << std::endl;
		file << "edge [colorscheme=set39];" << std::endl;

		for(auto& node : nodes) {
			file << "n" << node.id.rank() << "_" << node.id.id() << "["
				//<< "label=\"[" + std::to_string(node.id.rank()) + ":" + std::to_string(node.id.id()) + "]\","
				<< "label=\"\","
				<< "fixedsize=true,";
			if(node.fixed_position)
				// Located nodes = cells
				file
					<< "pos=\"" << node.x*2 << "," << node.y*2 << "!" << "\",";
			if(node.is_location)
				file
					<< "height=.3,width=.3,"
					<< "shape=diamond,";
			else
				file
					<< "height=.5,width=.5,";
			file
				<< "style=filled,"
				<< "fillcolor=" << node.rank+1 << ","
				<< "color=" << node.rank+1
				<< "];" << std::endl;
		}
		for(auto& edge : edges) {
			file
				<< "n" << edge.src.rank() << "_" << edge.src.id() << " -> "
				<< "n" << edge.tgt.rank() << "_" << edge.tgt.id() << " "
				<< "[color=" << 6+edge.layer << ",";
			if(
					edge.layer == fpmas::api::model::LOCATION ||
					edge.layer == fpmas::api::model::PERCEPTION ||
					edge.layer == CONTACT) {
				if(edge.layer == fpmas::api::model::LOCATION) {
					file << "len=0.1,weight=10000,";
				}
				file
					<< "style=bold,";
			} else {
				file
					<< "style=solid,";
			}
			file << "];" << std::endl;
		}

		file << "}" << std::endl;
	}
}

void to_json(nlohmann::json& json, const NodeView& node_view) {
	json[0] = node_view.id;
	json[1] = node_view.rank;
	json[2] = node_view.is_location;
	json[3] = node_view.fixed_position;
	if(node_view.fixed_position) {
		json[4] = node_view.x;
		json[5] = node_view.y;
	}
}

void from_json(const nlohmann::json& json, NodeView& node_view) {
	json[0].get_to(node_view.id);
	json[1].get_to(node_view.rank);
	json[2].get_to(node_view.is_location);
	json[3].get_to(node_view.fixed_position);
	if(node_view.fixed_position) {
		json[4].get_to(node_view.x);
		json[5].get_to(node_view.y);
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
