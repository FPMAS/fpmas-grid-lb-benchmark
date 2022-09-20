#include "interactions.h"
#include "metamodel.h"

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

void graph_stats_output(
		BasicMetaModel& model,
		std::string file_name
		) {
	fpmas::scheduler::Scheduler scheduler;
	fpmas::runtime::Runtime runtime(scheduler);
	fpmas::model::RandomLoadBalancing random_lb(fpmas::communication::WORLD);

	float C = fpmas::graph::clustering_coefficient(
			model.getModel().graph(), fpmas::api::model::CELL_SUCCESSOR);
	float L = fpmas::graph::characteristic_path_length(
			model.getModel().graph(), fpmas::api::model::CELL_SUCCESSOR,
			fpmas::model::local_agent_ids(model.cellGroup()));
	
	FPMAS_ON_PROC(model.getModel().getMpiCommunicator(), 0) {
		fpmas::io::FileOutput csv_output(file_name);
		fpmas::io::CsvOutput<float, float> graph_stats_csv(
				csv_output,
				{"C", [C] () {return C;}},
				{"L", [L] () {return L;}}
				);
		graph_stats_csv.dump();
	}
}

LoadBalancingCsvOutput::LoadBalancingCsvOutput(
		BasicMetaModel& metamodel,
		fpmas::api::utils::perf::Probe& balance_probe,
		fpmas::api::utils::perf::Probe& distribute_probe,
		fpmas::api::utils::perf::Probe& local_read_probe,
		fpmas::api::utils::perf::Probe& local_write_probe,
		fpmas::api::utils::perf::Probe& distant_read_probe,
		fpmas::api::utils::perf::Probe& distant_write_probe,
		fpmas::api::utils::perf::Probe& sync_probe,
		fpmas::api::utils::perf::Monitor& monitor) :
		fpmas::io::FileOutput(
				metamodel.getName() + ".%r.csv",
				metamodel.getModel().getMpiCommunicator().getRank()),
		LbCsvOutput(*this,
			{"TIME", [&metamodel] {return metamodel.getModel().runtime().currentDate();}},
			{"BALANCE_TIME", [&monitor] {
			auto result = std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("BALANCE")
					).count();
			return result;
			}},
			{"DISTRIBUTE_TIME", [&monitor] {
			auto result = std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("DISTRIBUTE")
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
			{"LOCAL_CELL_EDGES", [&metamodel] {
			float total_weight = 0;
			for(auto cell : metamodel.cellGroup().localAgents())
				for(auto edge : cell->node()->getOutgoingEdges())
					if(edge->state() == fpmas::api::graph::LOCAL) {
						total_weight+=edge->getWeight();
					}
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
			{"LOCAL_CELL_READ", [&monitor] {
			return std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("LOCAL_READ")
					).count();
			}},
			{"LOCAL_CELL_WRITE", [&monitor] {
			return std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("LOCAL_WRITE")
					).count();
			}},
			{"DISTANT_CELL_READ", [&monitor] {
			return std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("DISTANT_READ")
					).count();
			}},
			{"DISTANT_CELL_WRITE", [&monitor] {
			return std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("DISTANT_WRITE")
					).count();
			}},
			{"CELL_SYNC", [&monitor] {
			return std::chrono::duration_cast<std::chrono::microseconds>(
					monitor.totalDuration("SYNC")
					).count();
			}}
), commit_probes_task([
		&balance_probe, &distribute_probe,
		&local_read_probe, &local_write_probe,
		&distant_read_probe, &distant_write_probe,
		&sync_probe, &monitor
	] () {
	monitor.commit(balance_probe);
	monitor.commit(distribute_probe);
	monitor.commit(local_read_probe);
	monitor.commit(local_write_probe);
	monitor.commit(distant_read_probe);
	monitor.commit(distant_write_probe);
	monitor.commit(sync_probe);
	}),
	clear_monitor_task([&monitor] () {
			monitor.clear();
			}) {
	}
	
CellsOutput::CellsOutput(BasicMetaModel& meta_model,
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

void CellsOutput::dump() {
	auto cells = gather_cells();
	FPMAS_ON_PROC(meta_model.getModel().getMpiCommunicator(), 0) {
		nlohmann::json j = cells;
		output_file.get() << j.dump();
	}
}

std::vector<std::vector<int>> CellsOutput::gather_cells() {
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
	id(agent->node()->getId()),
	contacts(agent->contacts()) {
		for(auto perception : agent->node()->getOutgoingEdges(fpmas::api::model::PERCEPTION))
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
		BasicMetaModel& meta_model,
		std::string lb_algorithm,
		std::size_t grid_width, std::size_t grid_height
		) :
	fpmas::io::JsonOutput<AgentsOutputView>(
			output_file, [this, &meta_model, grid_width, grid_height] () {

			std::vector<MetaGridAgentView> local_agents;
			std::vector<DistantAgentView> distant_agents;
			for(auto agent : meta_model.agentGroup().agents())
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
					meta_model.getModel().getMpiCommunicator().getRank(),
					grid_width, grid_height, local_agents, distant_agents
					);
			}
			),
	output_file(
			lb_algorithm + "_agents.%r.%t.json",
			meta_model.getModel().getMpiCommunicator(), meta_model.getModel().runtime()
			),
	meta_model(meta_model), grid_width(grid_width), grid_height(grid_height) {
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
