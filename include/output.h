#include "fpmas/io/json_output.h"
#include "agent.h"

void dump_grid(
		std::size_t grid_width, std::size_t grid_height,
		std::vector<BenchmarkCell*> local_cells
		);

class TestCase;

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
			LoadBalancingCsvOutput(TestCase& test_case);
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

struct BenchmarkAgentView {
	DistributedId id;
	std::deque<DistributedId> contacts;
	std::vector<DistributedId> perceptions;
	DiscretePoint location;

	BenchmarkAgentView(const BenchmarkAgent* agent);
};

struct DistantBenchmarkAgentView {
	DistributedId id;
	int rank;

	DistantBenchmarkAgentView(const BenchmarkAgent* agent);
};

struct AgentsOutputView {
	int rank;
	std::size_t grid_width;
	std::size_t grid_height;
	std::vector<BenchmarkAgentView> agents;
	std::vector<DistantBenchmarkAgentView> distant_agents;

	AgentsOutputView(
			int rank, std::size_t grid_width, std::size_t grid_height,
			std::vector<BenchmarkAgentView> agents,
			std::vector<DistantBenchmarkAgentView> distant_agents
			);
};

namespace nlohmann {
	template<>
		struct adl_serializer<BenchmarkAgentView> {
			static void to_json(nlohmann::json& json, const BenchmarkAgentView& agent);
		};

	template<>
		struct adl_serializer<DistantBenchmarkAgentView> {
			static void to_json(nlohmann::json& json, const DistantBenchmarkAgentView& agent);
		};

	template<>
		struct adl_serializer<AgentsOutputView> {
			static void to_json(nlohmann::json& json, const AgentsOutputView& agent_output);
		};
}

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
		/*
		 * Returns a matrix containing the count of agents in each cell
		 */
		std::vector<std::vector<std::size_t>> gather_agents();
	public:
		AgentsOutput(
				fpmas::api::model::Model& model,
				std::string lb_algorithm,
				std::size_t grid_width, std::size_t grid_height
				);
};
