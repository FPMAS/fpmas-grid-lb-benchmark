#pragma once

#include "fpmas/io/json_output.h"
#include "agent.h"
#include <fpmas/io/output.h>

/**
 * @file output.h
 * Contains features used to perform MetaModel outputs.
 */

class BasicMetaModelFactory;
class BasicMetaModel;


/**
 * MetaModel CSV output.
 *
 * The following fields are written by each process to its own CSV file:
 * - `TIME`: current time step
 * - `BALANCE_TIME`: time required to apply the load balancing algorithm and
 *   distributed the model according to the new partition. Includes
 *   DISTRIBUTE_TIME.
 * - `DISTRIBUTE_TIME`: time required to distribute the model
 * - `AGENTS`: total weight of LOCAL agents
 * - `CELLS`: total weight of LOCAL cells
 * - `DISTANT_AGENT_EDGES`: total weight of DISTANT edges between two agents
 *   (PERCEPTION)
 * - `DISTANT_AGENT_CELL_EDGES`: total weight of DISTANT edges between an agent
 *   and a cell (LOCATION + PERCEIVE + MOVE)
 * - `DISTANT_CELL_EDGES`: total weight of DISTANT edges between two cells
 *   (CELL_SUCCESSOR)
 * - `LOCAL_CELL_READ_TIME`: total time spent in read operations between two LOCAL
 *   cells.
 * - `LOCAL_CELL_READ_COUNT`: total count of read operations between two LOCAL
 *   cells.
 * - `LOCAL_CELL_WRITE_TIME`: total time spent in write operations between two LOCAL
 *   cells.
 * - `LOCAL_CELL_WRITE_COUNT`: total count of write operations between two LOCAL
 *   cells.
 * - `DISTANT_CELL_READ_TIME`: total time spent in read operations from a LOCAL to
 *   a DISTANT cell.
 * - `DISTANT_CELL_READ_COUNT`: total count of read operations from a LOCAL to a
 *   DISTANT cell.
 * - `DISTANT_CELL_WRITE_TIME`: total time spent in write operations from a LOCAL
 *   to a DISTANT cell.
 * - `DISTANT_CELL_WRITE_COUNT`: total count of write operations from a LOCAL to a
 *   DISTANT cell.
 * - `CELL_SYNC`: total time spent synchronizing read/write operations between
 *   cells.
 */
class MetaModelCsvOutput :
	public fpmas::io::FileOutput,
	public fpmas::io::CsvOutput<
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
	> {
		private:
			fpmas::scheduler::detail::LambdaTask commit_probes_task;
			fpmas::scheduler::detail::LambdaTask clear_monitor_task;
			fpmas::scheduler::Job commit_probes_job {{commit_probes_task}};
			fpmas::scheduler::Job clear_monitor_job {{clear_monitor_task}};
			fpmas::scheduler::JobList _jobs;

		public:
			/**
			 * MetaModelCsvOutput constructor.
			 *
			 * The name of the output CSV file is set as
			 * "[model name].\%r.csv" where \%r is the rank of the current
			 * process.
			 *
			 * @param meta_model Model from which data is gathered
			 * @param balance_probe `BALANCE_TIME` probe
			 * @param distribute_probe `DISTRIBUTE_TIME` probe
			 * @param local_read_probe `LOCAL_CELL_[READ/COUNT]_TIME` probe
			 * @param local_write_probe `LOCAL_CELL_[WRITE/COUNT]_TIME` probe
			 * @param distant_read_probe `DISTANT_CELL_[READ/COUNT]_TIME` probe
			 * @param distant_write_probe `DISTANT_CELL_[WRITE/COUNT]_TIME` probe
			 * @param sync_probe `CELL_SYNC` probe
			 * @param monitor Monitor used to manage probes
			 */
			MetaModelCsvOutput(
					BasicMetaModel& meta_model,
					fpmas::api::utils::perf::Probe& balance_probe,
					fpmas::api::utils::perf::Probe& distribute_probe,
					fpmas::api::utils::perf::Probe& local_read_probe,
					fpmas::api::utils::perf::Probe& local_write_probe,
					fpmas::api::utils::perf::Probe& distant_read_probe,
					fpmas::api::utils::perf::Probe& distant_write_probe,
					fpmas::api::utils::perf::Probe& sync_probe,
					fpmas::api::utils::perf::Monitor& monitor
					);

			/**
			 * Job to schedule at each iteration to dump CSV data.
			 */
			const fpmas::scheduler::JobList& jobs() {
				return _jobs;
			}
	};


/**
 * Dumps the current grid to a `grid.json` file on the process 0 as a 2
 * dimension array such as `json[y][x]` contains the utility of the grid cell at
 * (x,y).
 *
 * Since the utility of the cells is currently static, it is not relevant to
 * periodically schedule this output, but dump() or job() can be used to perform
 * an utility output at the beginning of the simulation.
 */
class CellsUtilityOutput : public fpmas::io::OutputBase {
	private:
		fpmas::io::FileOutput output_file;
		BasicMetaModel& model;
		std::size_t grid_width;
		std::size_t grid_height;

	public:
		/**
		 * CellsUtilityOutput constructor.
		 *
		 * @param model MetaModel from which cells are gathered
		 * @param grid_width Width of the grid
		 * @param grid_height Height of the grid
		 */
		CellsUtilityOutput(
				BasicMetaModel& model,
				std::size_t grid_width,
				std::size_t grid_height) :
			fpmas::io::OutputBase(output_file),
			output_file("grid.json"),
			model(model), grid_width(grid_width), grid_height(grid_height) {
		}

		/**
		 * Dumps data from the MetaModel to the `grid.json` file.
		 *
		 * Must be called on **all** processes.
		 */
		void dump() override;
};

/**
 * Dumps the current grid to a JSON file on the process 0 as a 2 dimension array
 * such as `json[y][x]` contains the process on which the grid cell at (x,y) is
 * executed.
 *
 * This is currently only available for GRID Environments, to perform
 * visualisation in a Matplotlib pcolormesh for example. In order to visualise
 * the process of each cell in a generic graph environment, consider using the
 * DotOuput instead.
 */
class CellsLocationOutput : public fpmas::io::OutputBase {
	private:
		fpmas::io::DynamicFileOutput output_file;
		BasicMetaModel& meta_model;
		std::size_t grid_width;
		std::size_t grid_height;

		std::vector<std::vector<int>> gather_cells();

	public:
		/**
		 * CellsLocationOutput contructor.
		 *
		 * @param meta_model Model from which grid cells are gathered.
		 * @param filename Name of the output file
		 * @param grid_width Width of the grid
		 * @param grid_height Height of the grid
		 */
		CellsLocationOutput(
				BasicMetaModel& meta_model,
				std::string filename,
				std::size_t grid_width, std::size_t grid_height
				);

		/**
		 * Dumps data from the MetaModel to the JSON file.
		 *
		 * Must be called on **all** processes.
		 */
		void dump() override;
};

/**
 * Outputs the clustering coefficient and the characteristic path length of the
 * global cell network to a CSV file.
 *
 * Notice that computing the characteristic path length can be **very** costly.
 *
 * dump() or job() methods can be used to perform static or dynamic graph stats
 * output.
 */
class GraphStatsOutput : public fpmas::io::DistributedCsvOutput<
						 // Two local fields, dumped only on process 0
						 fpmas::io::Local<float>,
						 fpmas::io::Local<float>
							  > {
	private:
		fpmas::io::FileOutput output_file;

	public:
		/**
		 * GraphStatsOutput constructor.
		 *
		 * @param model Distributed spatial model
		 * @param file_name Name of the CSV file
		 */
		GraphStatsOutput(
				BasicMetaModel& model,
				std::string file_name
				);
};

/**
 * MetaAgent output helper, that can directly be serialized to json.
 */
struct MetaAgentView {
	/**
	 * ID of the agent.
	 */
	DistributedId id;
	/**
	 * IDs of the agent's contacts.
	 */
	std::deque<DistributedId> contacts;
	/**
	 * IDs of the agent's perceptions.
	 */
	std::vector<DistributedId> perceptions;

	/**
	 * MetaAgentView constructor.
	 *
	 * @param agent MetaAgent to output
	 */
	MetaAgentView(const MetaAgentBase* agent);
};

/**
 * MetaAgentView extension for MetaGridAgents.
 */
struct MetaGridAgentView : public MetaAgentView {
	/**
	 * Discrete location of the agent.
	 */
	DiscretePoint location;

	/**
	 * MetaGridAgentView constructor.
	 *
	 * @param agent MetaGridAgent to output
	 */
	MetaGridAgentView(const MetaGridAgent* agent);
};

/**
 * Distant agent output helper.
 */
struct DistantAgentView {
	/**
	 * ID of the agent.
	 */
	DistributedId id;
	/**
	 * Location of the DISTANT agent.
	 */
	int rank;

	/**
	 * DistantAgentView constructor.
	 *
	 * @param agent DISTANT agent to output
	 */
	DistantAgentView(const fpmas::api::model::Agent* agent);
};

/**
 * Helper class to serialize agents of a MetaGridModel.
 */
struct AgentsOutputView {
	/**
	 * Current process rank.
	 */
	int rank;
	/**
	 * Grid width.
	 */
	std::size_t grid_width;
	/**
	 * Grid height.
	 */
	std::size_t grid_height;
	/**
	 * List of LOCAL agents view.
	 */
	std::vector<MetaGridAgentView> agents;
	/**
	 * List of DISTANT agents view.
	 */
	std::vector<DistantAgentView> distant_agents;

	/**
	 * AgentsOutputView constructor.
	 */
	AgentsOutputView(
			int rank, std::size_t grid_width, std::size_t grid_height,
			std::vector<MetaGridAgentView> agents,
			std::vector<DistantAgentView> distant_agents
			);
};


/**
 * Performs the following Json output on each process:
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
 * ```
 */
class AgentsOutput : public fpmas::io::JsonOutput<AgentsOutputView> {
	private:
		fpmas::io::DynamicFileOutput output_file;

	public:
		/**
		 * AgentsOutput constructor.
		 */
		AgentsOutput(
				BasicMetaModel& model,
				std::size_t grid_width, std::size_t grid_height
				);
};

namespace nlohmann {
	/**
	 * MetaAgentView JSON serialization rules.
	 */
	template<>
		struct adl_serializer<MetaAgentView> {
			/**
			 * JSON serialization.
			 */
			static void to_json(nlohmann::json& json, const MetaAgentView& agent);
		};
	/**
	 * MetaGridAgentView JSON serialization.
	 */
	template<>
		struct adl_serializer<MetaGridAgentView> {
			/**
			 * JSON serialization.
			 */
			static void to_json(nlohmann::json& json, const MetaGridAgentView& agent);
		};
	/**
	 * DistantAgentView JSON serialization.
	 */
	template<>
		struct adl_serializer<DistantAgentView> {
			/**
			 * JSON serialization.
			 */
			static void to_json(nlohmann::json& json, const DistantAgentView& agent);
		};
	/**
	 * AgentsOutputView JSON serialization.
	 */
	template<>
		struct adl_serializer<AgentsOutputView> {
			/**
			 * JSON serialization.
			 */
			static void to_json(nlohmann::json& json, const AgentsOutputView& agent_output);
		};
}
