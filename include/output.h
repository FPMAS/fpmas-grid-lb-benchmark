#pragma once

#include "fpmas/io/json_output.h"
#include "agent.h"

void dump_grid(
		std::size_t grid_width, std::size_t grid_height,
		std::vector<MetaGridCell*> local_cells
		);

class BasicMetaModel;

typedef fpmas::io::CsvOutput<
fpmas::scheduler::Date, // Time Step
	unsigned int, // Partitioning time
	unsigned int, // Distribution time
	float, // Local Agents
	float, // Local cells
	float, // Distant agent edges
	float // Distant cell edges
	> LbCsvOutput;

class LoadBalancingCsvOutput :
	public fpmas::io::FileOutput,
	public LbCsvOutput {
		private:
			fpmas::io::FileOutput file;
		public:
			LoadBalancingCsvOutput(BasicMetaModel& test_case);
	};

class CellsOutput : public fpmas::io::OutputBase {
	private:
		fpmas::io::DynamicFileOutput output_file;
		fpmas::api::model::Model& model;
		std::size_t grid_width;
		std::size_t grid_height;

		std::vector<std::vector<int>> gather_cells();

	public:
		CellsOutput(
				fpmas::api::model::Model& model,
				std::string filename,
				std::size_t grid_width, std::size_t grid_height
				)
			:
				fpmas::io::OutputBase(output_file),
				output_file(
						filename + "_cells.%t.json",
						model.getMpiCommunicator(),
						model.runtime()
						),
				model(model), grid_width(grid_width), grid_height(grid_height) {
				}

		void dump() override;
};

struct MetaAgentView {
	DistributedId id;
	std::deque<DistributedId> contacts;
	std::vector<DistributedId> perceptions;
	DiscretePoint location;

	MetaAgentView(const MetaAgent* agent);
};

struct DistantMetaAgentView {
	DistributedId id;
	int rank;

	DistantMetaAgentView(const MetaAgent* agent);
};

struct AgentsOutputView {
	int rank;
	std::size_t grid_width;
	std::size_t grid_height;
	std::vector<MetaAgentView> agents;
	std::vector<DistantMetaAgentView> distant_agents;

	AgentsOutputView(
			int rank, std::size_t grid_width, std::size_t grid_height,
			std::vector<MetaAgentView> agents,
			std::vector<DistantMetaAgentView> distant_agents
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
		fpmas::api::model::Model& model;
		std::size_t grid_width;
		std::size_t grid_height;

	public:
		AgentsOutput(
				fpmas::api::model::Model& model,
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
		struct adl_serializer<DistantMetaAgentView> {
			static void to_json(nlohmann::json& json, const DistantMetaAgentView& agent);
		};

	template<>
		struct adl_serializer<AgentsOutputView> {
			static void to_json(nlohmann::json& json, const AgentsOutputView& agent_output);
		};
}
