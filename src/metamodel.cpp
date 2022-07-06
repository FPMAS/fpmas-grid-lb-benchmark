#include "metamodel.h"

void MetaGridModel::buildCells(const BenchmarkConfig& config) {
	std::unique_ptr<UtilityFunction> utility_function;
	switch(config.utility) {
		case UNIFORM:
			utility_function.reset(new UniformUtility);
			break;
		case LINEAR:
			utility_function.reset(new LinearUtility);
			break;
		case INVERSE:
			utility_function.reset(new InverseUtility);
			break;
		case STEP:
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
