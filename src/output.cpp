#include "metamodel.h"

MetaAgentView::MetaAgentView(const MetaAgent* agent) :
	id(agent->node()->getId()),
	contacts(agent->contacts()),
	location(agent->locationPoint()) {
		for(auto perception : agent->node()->getOutgoingEdges(fpmas::api::model::PERCEPTION))
			perceptions.push_back(perception->getTargetNode()->getId());
	}

DistantMetaAgentView::DistantMetaAgentView(const MetaAgent* agent) :
	id(agent->node()->getId()),
	rank(agent->node()->location()) {
	}

AgentsOutputView::AgentsOutputView(
		int rank, std::size_t grid_width, std::size_t grid_height,
		std::vector<MetaAgentView> agents,
		std::vector<DistantMetaAgentView> distant_agents
		) :
	rank(rank), grid_width(grid_width), grid_height(grid_height),
	agents(agents), distant_agents(distant_agents) {
}

void dump_grid(
		std::size_t grid_width, std::size_t grid_height,
		std::vector<MetaGridCell*> local_cells) {
	typedef std::pair<fpmas::model::DiscretePoint, float> CellView;
	std::vector<CellView> local_cells_view;
	for(auto cell : local_cells)
		local_cells_view.push_back({cell->location(), cell->getUtility()});

	fpmas::communication::TypedMpi<std::vector<CellView>> cell_mpi(fpmas::communication::WORLD);
	auto global_cells = fpmas::communication::reduce(
			cell_mpi, 0, local_cells_view, fpmas::utils::Concat()
			);
	FPMAS_ON_PROC(fpmas::communication::WORLD, 0) {
		fpmas::io::FileOutput grid_file {"grid.json"};
		std::vector<std::vector<float>> grid;
		grid.resize(grid_height);
		for(auto& row : grid)
			row.resize(grid_width);

		for(auto cell : global_cells)
			grid[cell.first.y][cell.first.x] = cell.second;

		fpmas::io::JsonOutput<decltype(grid)>(grid_file, [&grid] () {return grid;})
			.dump();
	}
}

AgentsOutput::AgentsOutput(
		fpmas::api::model::Model& model,
		std::string lb_algorithm,
		std::size_t grid_width, std::size_t grid_height
		) :
	fpmas::io::JsonOutput<AgentsOutputView>(
			output_file, [this, &model, grid_width, grid_height] () {

			std::vector<MetaAgentView> local_agents;
			std::vector<DistantMetaAgentView> distant_agents;
			for(auto agent : this->model.getGroup(AGENT_GROUP).agents())
				switch(agent->node()->state()) {
					case fpmas::api::graph::LOCAL:
						local_agents.emplace_back(
								dynamic_cast<const MetaAgent*>(agent)
								);
					break;
					case fpmas::api::graph::DISTANT:
						distant_agents.emplace_back(
								dynamic_cast<const MetaAgent*>(agent)
								);
					break;
				}
			return AgentsOutputView(
					model.getMpiCommunicator().getRank(),
					grid_width, grid_height, local_agents, distant_agents
					);
			}
			),
	output_file(
			lb_algorithm + "_agents.%r.%t.json",
			model.getMpiCommunicator(), model.runtime()
			),
	model(model), grid_width(grid_width), grid_height(grid_height) {
	}


LoadBalancingCsvOutput::LoadBalancingCsvOutput(BasicMetaModel& metamodel)
	:
		fpmas::io::FileOutput(
				metamodel.getLoadBalancingAlgorithmeName() + ".%r.csv",
				metamodel.getModel().getMpiCommunicator().getRank()),
		LbCsvOutput(*this,
			{"TIME", [&metamodel] {return metamodel.getModel().runtime().currentDate();}},
			{"BALANCE_TIME", [&metamodel] {
			auto result = std::chrono::duration_cast<std::chrono::microseconds>(
					metamodel.getLoadBalancingProbe().monitor.totalDuration("BALANCE")
					).count();
			return result;
			}},
			{"DISTRIBUTE_TIME", [&metamodel] {
			auto result = std::chrono::duration_cast<std::chrono::microseconds>(
					metamodel.getLoadBalancingProbe().monitor.totalDuration("DISTRIBUTE")
					).count();
			metamodel.getLoadBalancingProbe().monitor.clear();
			return result;
			}},
			{"AGENTS", [&metamodel] {
			float total_weight = 0;
			for(auto agent : metamodel.getModel().getGroup(AGENT_GROUP).localAgents())
				total_weight += agent->node()->getWeight();
			return total_weight;
			}},
			{"CELLS", [&metamodel] {
			float total_weight = 0;
			for(auto agent : metamodel.getModel().getGroup(CELL_GROUP).localAgents())
				total_weight += agent->node()->getWeight();
			return total_weight;
			}},
			{"DISTANT_AGENT_EDGES", [&metamodel] {
			float total_weight = 0;
			for(auto agent : metamodel.getModel().getGroup(AGENT_GROUP).localAgents())
				for(auto edge : agent->node()->getOutgoingEdges())
					if(edge->state() == fpmas::api::graph::DISTANT)
						total_weight+=edge->getWeight();
			return total_weight;
			}},
			{"DISTANT_CELL_EDGES", [&metamodel] {
			float total_weight = 0;
			for(auto cell : metamodel.getModel().getGroup(CELL_GROUP).localAgents())
				for(auto edge : cell->node()->getOutgoingEdges())
					if(edge->state() == fpmas::api::graph::DISTANT)
						total_weight+=edge->getWeight();
			return total_weight;
			}}
) {
}


void CellsOutput::dump() {
	auto cells = gather_cells();
	FPMAS_ON_PROC(model.getMpiCommunicator(), 0) {
		nlohmann::json j = cells;
		output_file.get() << j.dump();
	}
}

std::vector<std::vector<int>> CellsOutput::gather_cells() {
	typedef std::pair<fpmas::model::DiscretePoint, int> CellLocation;

	std::vector<fpmas::api::model::Agent*> local_cells
		= model.getGroup(CELL_GROUP).localAgents();
	std::vector<CellLocation> local_cells_location;
	for(auto cell : local_cells)
		local_cells_location.push_back({
				dynamic_cast<MetaGridCell*>(cell)->location(),
				cell->node()->location()
				});

	fpmas::communication::TypedMpi<std::vector<CellLocation>> cell_mpi(fpmas::communication::WORLD);
	auto global_cells = fpmas::communication::reduce(
			cell_mpi, 0, local_cells_location, fpmas::utils::Concat()
			);

	std::vector<std::vector<int>> grid;
	FPMAS_ON_PROC(fpmas::communication::WORLD, 0) {
		grid.resize(grid_height);
		for(auto& row : grid)
			row.resize(grid_width);

		for(auto cell : global_cells)
			grid[cell.first.y][cell.first.x] = cell.second;
	}
	return grid;
}

namespace nlohmann {
	void adl_serializer<MetaAgentView>::to_json(
			nlohmann::json& j, const MetaAgentView &agent) {
		j["id"] = agent.id;
		j["contacts"] = agent.contacts;
		j["perceptions"] = agent.perceptions;
		j["location"] = agent.location;
	}

	void adl_serializer<DistantMetaAgentView>::to_json(
			nlohmann::json& j, const DistantMetaAgentView &agent) {
		j["id"] = agent.id;
		j["rank"] = agent.rank;
	}

	void adl_serializer<AgentsOutputView>::to_json(
			nlohmann::json& j, const AgentsOutputView& agent_view) {
		j["rank"] = agent_view.rank;
		j["grid"]["width"] = agent_view.grid_width;
		j["grid"]["height"] = agent_view.grid_height;
		j["agents"] = agent_view.agents;
		j["distant_agents"] = agent_view.distant_agents;
	}
}
