#include "metamodel.h"
#include "fpmas/model/spatial/graph_builder.h"
#include <fpmas/model/spatial/spatial_model.h>
#include <fpmas/random/distribution.h>

void MetaGridModel::buildCells(const BenchmarkConfig& config) {
	std::unique_ptr<UtilityFunction> utility_function;
	switch(config.utility) {
		case Utility::UNIFORM:
			utility_function.reset(new UniformUtility);
			break;
		case Utility::LINEAR:
			utility_function.reset(new LinearUtility);
			break;
		case Utility::INVERSE:
			utility_function.reset(new InverseUtility);
			break;
		case Utility::STEP:
			utility_function.reset(new StepUtility);
			break;
	}
	MetaGridCellFactory cell_factory(*utility_function, config.attractors);
	MooreGrid<MetaGridCell>::Builder grid(
			cell_factory, config.grid_width, config.grid_height);

	auto local_cells = grid.build(model, {model.getGroup(CELL_GROUP)});
	dump_grid(config.grid_width, config.grid_height, local_cells);
}

void MetaGridModel::buildAgents(const BenchmarkConfig& config) {
	fpmas::model::UniformGridAgentMapping mapping(
			config.grid_width, config.grid_height,
			config.grid_width * config.grid_height * config.occupation_rate
			);
	fpmas::model::GridAgentBuilder<MetaGridCell> agent_builder;
	fpmas::model::DefaultSpatialAgentFactory<MetaGridAgent> agent_factory;

	agent_builder.build(
			model,
			{
			model.getGroup(RELATIONS_FROM_NEIGHBORS_GROUP),
			model.getGroup(RELATIONS_FROM_CONTACTS_GROUP),
			model.getGroup(HANDLE_NEW_CONTACTS_GROUP),
			model.getGroup(MOVE_GROUP)
			},
			agent_factory, mapping);
}

void MetaGraphModel::buildCells(const BenchmarkConfig& config) {
	fpmas::random::PoissonDistribution<std::size_t> edge_dist(config.output_degree);
	fpmas::api::graph::DistributedGraphBuilder<fpmas::model::AgentPtr>* builder;
	switch(config.graph_type) {
		case GraphType::RANDOM:
			builder = new DistributedUniformGraphBuilder(edge_dist);
			break;
		case GraphType::CLUSTERED:
			builder = new DistributedClusteredGraphBuilder(edge_dist);
			break;
		case GraphType::SMALL_WORLD:
			builder = new SmallWorldGraphBuilder(0.1, config.output_degree);
			break;
	}
	SpatialGraphBuilder<MetaGraphCell> graph_builder(*builder, config.num_cells);
	graph_builder.build(model);
	delete builder;
}

void MetaGraphModel::buildAgents(const BenchmarkConfig& config) {
	fpmas::model::UniformAgentMapping mapping(
			this->getModel().getMpiCommunicator(),
			this->cellGroup(),
			config.num_cells * config.occupation_rate
			);
	fpmas::model::SpatialAgentBuilder<MetaGraphCell> agent_builder;
	fpmas::model::DefaultSpatialAgentFactory<MetaGraphAgent> agent_factory;
	agent_builder.build(
			model,
			{
			model.getGroup(RELATIONS_FROM_NEIGHBORS_GROUP),
			model.getGroup(RELATIONS_FROM_CONTACTS_GROUP),
			model.getGroup(HANDLE_NEW_CONTACTS_GROUP),
			model.getGroup(MOVE_GROUP)
			},
			agent_factory, mapping);
}
