#include "fpmas.h"
#include "metamodel.h"
#include "fpmas/model/spatial/cell_load_balancing.h"

FPMAS_BASE_DATAPACK_SET_UP(
		GridCell::JsonBase,
		GraphCell::JsonBase,
		MetaGridAgent::JsonBase,
		MetaGridCell::JsonBase,
		MetaGraphCell::JsonBase,
		MetaGraphAgent::JsonBase
		);

FPMAS_BASE_JSON_SET_UP(
		GridCell::JsonBase,
		GraphCell::JsonBase,
		MetaGridAgent::JsonBase,
		MetaGridCell::JsonBase,
		MetaGraphCell::JsonBase,
		MetaGraphAgent::JsonBase
		);

using namespace fpmas::synchro;

int main(int argc, char** argv) {
	FPMAS_REGISTER_AGENT_TYPES(
			GridCell::JsonBase,
			GraphCell::JsonBase,
			MetaGridAgent::JsonBase,
			MetaGridCell::JsonBase,
			MetaGraphCell::JsonBase,
			MetaGraphAgent::JsonBase
			);
	if(argc <= 1) {
		std::cerr << "[FATAL ERROR] Missing config file argument" << std::endl;
		return EXIT_FAILURE;
	}

	fpmas::init(argc, argv);
	{
		BenchmarkConfig config(argv[1]);
		if(!config.is_valid)
			return EXIT_FAILURE;

		BasicMetaModelFactory* model_factory;
		switch(config.environment) {
			case Environment::GRID:
				model_factory = new MetaModelFactory<MetaGridModel>;
				break;
			case Environment::GRAPH:
				model_factory = new MetaModelFactory<MetaGraphModel>;
				break;
		}

		for(auto test_case : config.test_cases) {
			for(auto lb_period : test_case.lb_periods) {
				fpmas::scheduler::Scheduler scheduler;
				fpmas::runtime::Runtime runtime(scheduler);

				switch(test_case.algorithm) {
					case LbAlgorithm::ZOLTAN_LB:
						{
							ZoltanLoadBalancing zoltan_lb(
									fpmas::communication::WORLD, lb_period);
							BasicMetaModel* model = model_factory->build(
									"zoltan_lb", config,
									scheduler, runtime, zoltan_lb, lb_period
									);
							model->init()->run();
							delete model;
						}
						break;
					case LbAlgorithm::SCHEDULED_LB:
						{
							ZoltanLoadBalancing zoltan_lb(
									fpmas::communication::WORLD, lb_period);
							ScheduledLoadBalancing scheduled_load_balancing(
									zoltan_lb, scheduler, runtime
									);
							BasicMetaModel* model = model_factory->build(
									"scheduled_lb", config,
									scheduler, runtime, scheduled_load_balancing,
									lb_period
									);
							model->init()->run();
							delete model;
						}
						break;
					case LbAlgorithm::GRID_LB:
						{
							GridLoadBalancing grid_lb(
									config.grid_width, config.grid_height,
									fpmas::communication::WORLD
									);
							BasicMetaModel* model = model_factory->build(
									"grid_lb", config,
									scheduler, runtime, grid_lb, lb_period
									);
							model->init()->run();
							delete model;
						}
						break;
					case LbAlgorithm::ZOLTAN_CELL_LB:
						{
							ZoltanLoadBalancing zoltan_lb(
									fpmas::communication::WORLD, lb_period);
							CellLoadBalancing zoltan_cell_lb(
									fpmas::communication::WORLD, zoltan_lb
									);
							BasicMetaModel* model = model_factory->build(
									"zoltan_cell_lb", config,
									scheduler, runtime, zoltan_cell_lb, lb_period
									);
							model->init()->run();
							delete model;
						}
						break;
					case LbAlgorithm::RANDOM_LB:
						{

							RandomLoadBalancing random_lb(fpmas::communication::WORLD);
							BasicMetaModel* model = model_factory->build(
									"random_lb", config,
									scheduler, runtime, random_lb, lb_period
									);
							model->init()->run();
							delete model;
						}
						break;
				}
			}
		}
		delete model_factory;
	}
	fpmas::finalize();
}
