#pragma once

#include "fpmas/io/json_output.h"
#include "agent.h"

void dump_grid(
		std::size_t grid_width, std::size_t grid_height,
		std::vector<MetaGridCell*> local_cells
		);

class BasicMetaModelFactory;
class BasicMetaModel;

void graph_stats_output(
		BasicMetaModel& model,
		std::string file_name
		);

typedef fpmas::io::CsvOutput<
fpmas::scheduler::Date, // Time Step
	unsigned int, // Partitioning time
	unsigned int, // Distribution time
	float, // Local Agents
	float, // Local cells
	float, // Distant agent->agent edges
	float, // Distant agent->cell edges
	float, // Distant cell->cell edges
	unsigned int, // Cell->Cell read counters
	unsigned int, // Cell->Cell write counters
	unsigned int // Sync time
	> LbCsvOutput;

class LoadBalancingCsvOutput :
	public fpmas::io::FileOutput,
	public LbCsvOutput {
		private:
			fpmas::scheduler::detail::LambdaTask commit_probes_task;
			fpmas::scheduler::detail::LambdaTask clear_monitor_task;

		public:
			fpmas::scheduler::Job commit_probes_job {{commit_probes_task}};
			fpmas::scheduler::Job clear_monitor_job {{clear_monitor_task}};
			LoadBalancingCsvOutput(
					BasicMetaModel& meta_model,
					fpmas::api::utils::perf::Probe& balance_probe,
					fpmas::api::utils::perf::Probe& distribute_probe,
					fpmas::api::utils::perf::Probe& read_probe,
					fpmas::api::utils::perf::Probe& write_probe,
					fpmas::api::utils::perf::Probe& sync_probe,
					fpmas::api::utils::perf::Monitor& monitor
					);
	};

class CellsOutput : public fpmas::io::OutputBase {
	private:
		fpmas::io::DynamicFileOutput output_file;
		BasicMetaModel& meta_model;
		std::size_t grid_width;
		std::size_t grid_height;

		std::vector<std::vector<int>> gather_cells();

	public:
		CellsOutput(
				BasicMetaModel& meta_model,
				std::string filename,
				std::size_t grid_width, std::size_t grid_height
				);

		void dump() override;
};

struct MetaAgentView {
	DistributedId id;
	std::deque<DistributedId> contacts;
	std::vector<DistributedId> perceptions;

	MetaAgentView(const MetaAgentBase* agent);
};

struct MetaGridAgentView : public MetaAgentView {
	DiscretePoint location;

	MetaGridAgentView(const MetaGridAgent* agent);
};

struct DistantAgentView {
	DistributedId id;
	int rank;

	DistantAgentView(const fpmas::api::model::Agent* agent);
};

struct AgentsOutputView {
	int rank;
	std::size_t grid_width;
	std::size_t grid_height;
	std::vector<MetaAgentView> agents;
	std::vector<DistantAgentView> distant_agents;

	AgentsOutputView(
			int rank, std::size_t grid_width, std::size_t grid_height,
			std::vector<MetaAgentView> agents,
			std::vector<DistantAgentView> distant_agents
			);
};


/**
 * Json serialization scheme:
 * ```
 * # Agents list
 * {
 *     "rank": rank,
 *     "grid": {
 *         "width": grid_width,
 *         "height": grid_height
 *     },
 *     "agents": [
 *         {
 *             "id": agent_id,
 *             "contacts": [ids, ...],
 *             "perceptions": [ids, ...],
 *             "location": [x, y]
 *         },
 *         ...
 *     ],
 *     "distant_agents": [
 *         {
 *             "id": agent_id,
 *             "rank": rank
 *         },
 *         ...
 *     ]
 * }
 */
class AgentsOutput : public fpmas::io::JsonOutput<AgentsOutputView> {
	private:
		fpmas::io::DynamicFileOutput output_file;
		BasicMetaModel& meta_model;
		std::size_t grid_width;
		std::size_t grid_height;

	public:
		AgentsOutput(
				BasicMetaModel& model,
				std::string lb_algorithm,
				std::size_t grid_width, std::size_t grid_height
				);
};

namespace nlohmann {
	template<>
		struct adl_serializer<MetaAgentView> {
			static void to_json(nlohmann::json& json, const MetaAgentView& agent);
		};
	template<>
		struct adl_serializer<MetaGridAgentView> {
			static void to_json(nlohmann::json& json, const MetaGridAgentView& agent);
		};

	template<>
		struct adl_serializer<DistantAgentView> {
			static void to_json(nlohmann::json& json, const DistantAgentView& agent);
		};

	template<>
		struct adl_serializer<AgentsOutputView> {
			static void to_json(nlohmann::json& json, const AgentsOutputView& agent_output);
		};
}
