#include "benchmark.h"

BenchmarkAgentView::BenchmarkAgentView(const BenchmarkAgent* agent) :
	id(agent->node()->getId()),
	contacts(agent->contacts()),
	location(agent->locationPoint()) {
		for(auto perception : agent->node()->getOutgoingEdges(fpmas::api::model::PERCEPTION))
			perceptions.push_back(perception->getTargetNode()->getId());
	}

DistantBenchmarkAgentView::DistantBenchmarkAgentView(const BenchmarkAgent* agent) :
	id(agent->node()->getId()),
	rank(agent->node()->location()) {
	}

AgentsOutputView::AgentsOutputView(
		int rank, std::size_t grid_width, std::size_t grid_height,
		std::vector<BenchmarkAgentView> agents,
		std::vector<DistantBenchmarkAgentView> distant_agents
		) :
	rank(rank), grid_width(grid_width), grid_height(grid_height),
	agents(agents), distant_agents(distant_agents) {
}

void dump_grid(
		std::size_t grid_width, std::size_t grid_height,
		std::vector<BenchmarkCell*> local_cells) {
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

			std::vector<BenchmarkAgentView> local_agents;
			std::vector<DistantBenchmarkAgentView> distant_agents;
			for(auto agent : this->model.getGroup(AGENT_GROUP).agents())
				switch(agent->node()->state()) {
					case fpmas::api::graph::LOCAL:
						local_agents.emplace_back(
								dynamic_cast<const BenchmarkAgent*>(agent)
								);
					break;
					case fpmas::api::graph::DISTANT:
						distant_agents.emplace_back(
								dynamic_cast<const BenchmarkAgent*>(agent)
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


LoadBalancingCsvOutput::LoadBalancingCsvOutput(TestCase& test_case)
	:
		fpmas::io::FileOutput(test_case.lb_algorithm_name + ".%r.csv", test_case.model.getMpiCommunicator().getRank()),
		LbCsvOutput(*this,
			{"TIME", [&test_case] {return test_case.model.runtime().currentDate();}},
			{"BALANCE_TIME", [&test_case] {
			auto result = std::chrono::duration_cast<std::chrono::microseconds>(
					test_case.lb_probe.monitor.totalDuration("BALANCE")
					).count();
			return result;
			}},
			{"DISTRIBUTE_TIME", [&test_case] {
			auto result = std::chrono::duration_cast<std::chrono::microseconds>(
					test_case.lb_probe.monitor.totalDuration("DISTRIBUTE")
					).count();
			test_case.lb_probe.monitor.clear();
			return result;
			}},
			{"AGENTS", [&test_case] {
			float total_weight = 0;
			for(auto agent : test_case.model.getGroup(AGENT_GROUP).localAgents())
				total_weight += agent->node()->getWeight();
			return total_weight;
			}},
			{"CELLS", [&test_case] {
			float total_weight = 0;
			for(auto agent : test_case.model.getGroup(CELL_GROUP).localAgents())
				total_weight += agent->node()->getWeight();
			return total_weight;
			}},
			{"DISTANT_AGENT_EDGES", [&test_case] {
			float total_weight = 0;
			for(auto agent : test_case.model.getGroup(AGENT_GROUP).localAgents())
				for(auto edge : agent->node()->getOutgoingEdges())
					if(edge->state() == fpmas::api::graph::DISTANT)
						total_weight+=edge->getWeight();
			return total_weight;
			}},
			{"DISTANT_CELL_EDGES", [&test_case] {
			float total_weight = 0;
			for(auto cell : test_case.model.getGroup(CELL_GROUP).localAgents())
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
				dynamic_cast<BenchmarkCell*>(cell)->location(),
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
	void adl_serializer<BenchmarkAgentView>::to_json(
			nlohmann::json& j, const BenchmarkAgentView &agent) {
		j["id"] = agent.id;
		j["contacts"] = agent.contacts;
		j["perceptions"] = agent.perceptions;
		j["location"] = agent.location;
	}

	void adl_serializer<DistantBenchmarkAgentView>::to_json(
			nlohmann::json& j, const DistantBenchmarkAgentView &agent) {
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
