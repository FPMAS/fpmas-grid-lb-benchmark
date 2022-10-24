#include "interactions.h"
#include "metamodel.h"

void CellsUtilityOutput::dump() {
	typedef std::pair<fpmas::model::DiscretePoint, float> CellView;
	std::vector<CellView> local_cells_view;
	for(auto agent : model.cellGroup().localAgents()) {
		auto cell = dynamic_cast<MetaGridCell*>(agent);
		local_cells_view.push_back({cell->location(), cell->getUtility()});
	}

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

GraphStatsOutput::GraphStatsOutput(
		BasicMetaModel& model,
		std::string file_name
		) :
	fpmas::io::DistributedCsvOutput<
		fpmas::io::Local<float>,
		fpmas::io::Local<float>>(
			model.getModel().getMpiCommunicator(), 0, output_file,
			{"C", [&model] () -> float{
			return fpmas::graph::clustering_coefficient(
			model.getModel().graph(), fpmas::api::model::CELL_SUCCESSOR);
			}},
			{ "L", [&model] () {
			return fpmas::graph::characteristic_path_length(
			model.getModel().graph(), fpmas::api::model::CELL_SUCCESSOR,
			fpmas::model::local_agent_ids(model.cellGroup()));
			}}
			),
	output_file(file_name) {
	}

MetaModelCsvOutput::MetaModelCsvOutput(
		BasicMetaModel& metamodel,
		fpmas::api::utils::perf::Probe& lb_algorithm_probe,
		fpmas::api::utils::perf::Probe& graph_balance_probe,
		fpmas::api::utils::perf::Probe& local_read_probe,
		fpmas::api::utils::perf::Probe& local_write_probe,
		fpmas::api::utils::perf::Probe& distant_read_probe,
		fpmas::api::utils::perf::Probe& distant_write_probe,
		fpmas::api::utils::perf::Probe& sync_probe,
		fpmas::api::utils::perf::Monitor& monitor) :
		fpmas::io::FileOutput(
				metamodel.getName() + ".%r.csv",
				metamodel.getModel().getMpiCommunicator().getRank()),
		fpmas::io::CsvOutput<
			fpmas::scheduler::Date, // Time Step
			unsigned int, // Partitioning time
			unsigned int, // Distribution time
			float, // Local Agents
			float, // Local cells
			float, // Distant agent->agent edges
			float, // Distant agent->cell edges
			float, // Distant cell->cell edges
			unsigned int, // LOCAL Cell->Cell read time
			unsigned int, // LOCAL Cell->Cell read count
			unsigned int, // LOCAL Cell->Cell write time
			unsigned int, // LOCAL Cell->Cell write count
			unsigned int, // DISTANT Cell->Cell read time
			unsigned int, // DISTANT Cell->Cell read count
			unsigned int, // DISTANT Cell->Cell write time
			unsigned int, // DISTANT Cell->Cell write count
			unsigned int // Sync time
		>(*this,
			{"TIME", [&metamodel] {return metamodel.getModel().runtime().currentDate();}},
			{"BALANCE_TIME", [&monitor] {
			auto result = std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("LB_ALGORITHM")
					).count();
			return result;
			}},
			{"DISTRIBUTE_TIME", [&monitor] {
			auto result = std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("GRAPH_BALANCE")
						- monitor.totalDuration("LB_ALGORITHM")
					).count();
			return result;
			}},
			{"AGENTS", [&metamodel] {
			float total_weight = 0;
			for(auto agent : metamodel.agentGroup().localAgents())
				total_weight += agent->node()->getWeight();
			return total_weight;
			}},
			{"CELLS", [&metamodel] {
			float total_weight = 0;
			for(auto agent : metamodel.cellGroup().localAgents())
				total_weight += agent->node()->getWeight();
			return total_weight;
			}},
			{"DISTANT_AGENT_EDGES", [&metamodel] {
			float total_weight = 0;
			for(auto agent : metamodel.agentGroup().localAgents()) {
				for(auto edge : agent->node()->getOutgoingEdges())
					if(edge->state() == fpmas::api::graph::DISTANT
							&& dynamic_cast<MetaAgentBase*>(edge->getTargetNode()->data().get()))
						total_weight+=edge->getWeight();
			}
			return total_weight;
			}},
			{"DISTANT_AGENT_CELL_EDGES", [&metamodel] {
			float total_weight = 0;
			for(auto agent : metamodel.agentGroup().localAgents())
				for(auto edge : agent->node()->getOutgoingEdges())
					if(edge->state() == fpmas::api::graph::DISTANT
							&& dynamic_cast<MetaCell*>(edge->getTargetNode()->data().get()))
						total_weight+=edge->getWeight();
			return total_weight;
			}},
			{"DISTANT_CELL_EDGES", [&metamodel] {
			float total_weight = 0;
			for(auto cell : metamodel.cellGroup().localAgents())
				for(auto edge : cell->node()->getOutgoingEdges())
					// No filter needed since there is no outgoing edges to
					// other agents than cells
					if(edge->state() == fpmas::api::graph::DISTANT) {
						total_weight+=edge->getWeight();
					}
			return total_weight;
			}},
			{"LOCAL_CELL_READ_TIME", [&monitor] {
			return std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("LOCAL_READ")
					).count();
			}},
			{"LOCAL_CELL_READ_COUNT", [&monitor] {
			return monitor.callCount("LOCAL_READ");
			}},
			{"LOCAL_CELL_WRITE_TIME", [&monitor] {
			return std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("LOCAL_WRITE")
					).count();
			}},
			{"LOCAL_CELL_WRITE_COUNT", [&monitor] {
			return monitor.callCount("LOCAL_WRITE");
			}},
			{"DISTANT_CELL_READ_TIME", [&monitor] {
			return std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("DISTANT_READ")
					).count();
			}},
			{"DISTANT_CELL_READ_COUNT", [&monitor] {
			return monitor.callCount("DISTANT_READ");
			}},
			{"DISTANT_CELL_WRITE_TIME", [&monitor] {
			return std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("DISTANT_WRITE")
					).count();
			}},
			{"DISTANT_CELL_WRITE_COUNT", [&monitor] {
			return monitor.callCount("DISTANT_WRITE");
			}},
			{"CELL_SYNC", [&monitor] {
			return std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("SYNC")
					).count();
			}}
	), commit_probes_task([
		&lb_algorithm_probe, &graph_balance_probe,
		&local_read_probe, &local_write_probe,
		&distant_read_probe, &distant_write_probe,
		&sync_probe, &monitor
	] () {
		monitor.commit(lb_algorithm_probe);
		monitor.commit(graph_balance_probe);
		monitor.commit(local_read_probe);
		monitor.commit(local_write_probe);
		monitor.commit(distant_read_probe);
		monitor.commit(distant_write_probe);
		monitor.commit(sync_probe);
	}),
	clear_monitor_task([&monitor] () {
		monitor.clear();
	}),
	_jobs({commit_probes_job, this->job(), clear_monitor_job}){
	}
	
CellsLocationOutput::CellsLocationOutput(BasicMetaModel& meta_model,
		std::string filename,
		std::size_t grid_width, std::size_t grid_height
		) :
	fpmas::io::OutputBase(output_file),
	output_file(
			filename + "_cells.%t.json",
			meta_model.getModel().getMpiCommunicator(),
			meta_model.getModel().runtime()
			),
	meta_model(meta_model), grid_width(grid_width), grid_height(grid_height) {
	}

void CellsLocationOutput::dump() {
	auto cells = gather_cells();
	FPMAS_ON_PROC(meta_model.getModel().getMpiCommunicator(), 0) {
		nlohmann::json j = cells;
		output_file.get() << j.dump();
	}
}

std::vector<std::vector<int>> CellsLocationOutput::gather_cells() {
	typedef std::pair<fpmas::model::DiscretePoint, int> CellLocation;

	std::vector<fpmas::api::model::Agent*> local_cells
		= meta_model.cellGroup().localAgents();
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

MetaAgentView::MetaAgentView(const MetaAgentBase* agent) :
	id(agent->agentNode()->getId()),
	contacts(agent->contacts()) {
		for(auto perception : agent->agentNode()->getOutgoingEdges(fpmas::api::model::PERCEPTION))
			perceptions.push_back(perception->getTargetNode()->getId());
	}

MetaGridAgentView::MetaGridAgentView(const MetaGridAgent* agent)
	: MetaAgentView(agent), location(agent->locationPoint()) {
	}

DistantAgentView::DistantAgentView(const fpmas::api::model::Agent* agent) :
	id(agent->node()->getId()),
	rank(agent->node()->location()) {
	}

AgentsOutputView::AgentsOutputView(
		int rank, std::size_t grid_width, std::size_t grid_height,
		std::vector<MetaGridAgentView> agents,
		std::vector<DistantAgentView> distant_agents
		) :
	rank(rank), grid_width(grid_width), grid_height(grid_height),
	agents(agents), distant_agents(distant_agents) {
}

AgentsOutput::AgentsOutput(
		BasicMetaModel& model,
		std::size_t grid_width, std::size_t grid_height
		) :
	fpmas::io::JsonOutput<AgentsOutputView>(
			output_file, [this, &model, grid_width, grid_height] () {

			std::vector<MetaGridAgentView> local_agents;
			std::vector<DistantAgentView> distant_agents;
			for(auto agent : model.agentGroup().agents())
				switch(agent->node()->state()) {
					case fpmas::api::graph::LOCAL:
						local_agents.emplace_back(
								dynamic_cast<const MetaGridAgent*>(agent)
								);
					break;
					case fpmas::api::graph::DISTANT:
						distant_agents.emplace_back(agent);
					break;
				}
			return AgentsOutputView(
					model.getModel().getMpiCommunicator().getRank(),
					grid_width, grid_height, local_agents, distant_agents
					);
			}
			),
	output_file(
			model.getName() + "_agents.%r.%t.json",
			model.getModel().getMpiCommunicator(), model.getModel().runtime()
			) {
	}

namespace nlohmann {
	void adl_serializer<MetaAgentView>::to_json(
			nlohmann::json& j, const MetaAgentView &agent) {
		j["id"] = agent.id;
		j["contacts"] = agent.contacts;
		j["perceptions"] = agent.perceptions;
	}

	void adl_serializer<MetaGridAgentView>::to_json(
			nlohmann::json& j, const MetaGridAgentView &agent) {
		j = (const MetaAgentView&) agent;
		j["location"] = agent.location;
	}

	void adl_serializer<DistantAgentView>::to_json(
			nlohmann::json& j, const DistantAgentView &agent) {
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
