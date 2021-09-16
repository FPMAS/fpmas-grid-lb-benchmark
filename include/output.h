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

class AgentsOutput : public fpmas::io::JsonOutput<std::vector<std::vector<std::size_t>>> {
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
				) :
			fpmas::io::JsonOutput<std::vector<std::vector<std::size_t>>>(
					output_file, [this] () {return gather_agents();}
					),
			output_file(
					lb_algorithm + "_agents.%r.%t.json",
					model.getMpiCommunicator(), model.runtime()
					),
			model(model), grid_width(grid_width), grid_height(grid_height) {
			}
};

