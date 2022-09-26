#include "fpmas.h"
#include "metamodel.h"
#include "fpmas/model/spatial/cell_load_balancing.h"
#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"
#include "yaml-cpp/node/parse.h"


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
	CLI::App app("fpmas-metamodel");
	std::string config_file;
	app.add_option("config_file", config_file, "Metamodel YAML configuration file")
		->required();
	unsigned long seed = fpmas::random::default_seed;
	app.add_option("-s,--seed", seed, "Random seed");

	CLI11_PARSE(app, argc, argv);

	fpmas::seed(seed);
	fpmas::init(argc, argv);
	{
		BenchmarkConfig config(YAML::LoadFile(config_file));
		if(!config.is_valid)
			return EXIT_FAILURE;

		MetaModelFactory model_factory(config.environment, config.sync_mode);

		for(auto test_case : config.test_cases) {
			for(auto lb_period : test_case.lb_periods) {
				fpmas::scheduler::Scheduler scheduler;
				fpmas::runtime::Runtime runtime(scheduler);

				switch(test_case.algorithm) {
					case LbAlgorithm::ZOLTAN_LB:
						{
							ZoltanLoadBalancing zoltan_lb(
									fpmas::communication::WORLD, lb_period, config.zoltan_imbalance_tol);
							BasicMetaModel* model = model_factory.build(
									"zoltan_lb-" + std::to_string(lb_period), config,
									scheduler, runtime, zoltan_lb, lb_period
									);
							model->init()->run();
							delete model;
						}
						break;
					case LbAlgorithm::SCHEDULED_LB:
						{
							ZoltanLoadBalancing zoltan_lb(
									fpmas::communication::WORLD, lb_period, config.zoltan_imbalance_tol);
							ScheduledLoadBalancing scheduled_load_balancing(
									zoltan_lb, scheduler, runtime
									);
							BasicMetaModel* model = model_factory.build(
									"scheduled_lb-" + std::to_string(lb_period), config,
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
							BasicMetaModel* model = model_factory.build(
									"grid_lb-" + std::to_string(lb_period), config,
									scheduler, runtime, grid_lb, lb_period
									);
							model->init()->run();
							delete model;
						}
						break;
					case LbAlgorithm::ZOLTAN_CELL_LB:
						{
							ZoltanLoadBalancing zoltan_lb(
									fpmas::communication::WORLD, lb_period, config.zoltan_imbalance_tol);
							CellLoadBalancing zoltan_cell_lb(
									fpmas::communication::WORLD, zoltan_lb
									);
							BasicMetaModel* model = model_factory.build(
									"zoltan_cell_lb-" + std::to_string(lb_period), config,
									scheduler, runtime, zoltan_cell_lb, lb_period
									);
							model->init()->run();
							delete model;
						}
						break;
					case LbAlgorithm::STATIC_ZOLTAN_CELL_LB:
						{
							ZoltanLoadBalancing zoltan_lb(
									fpmas::communication::WORLD, lb_period, config.zoltan_imbalance_tol);
							StaticCellLoadBalancing static_cell_lb(fpmas::communication::WORLD, zoltan_lb);
							BasicMetaModel* model = model_factory.build(
									"static_zoltan_cell_lb-" + std::to_string(lb_period), config,
									scheduler, runtime, static_cell_lb, lb_period
									);
							model->init()->run();
							delete model;
						}
						break;
					case LbAlgorithm::RANDOM_LB:
						{

							RandomLoadBalancing random_lb(fpmas::communication::WORLD);
							BasicMetaModel* model = model_factory.build(
									"random_lb-" + std::to_string(lb_period), config,
									scheduler, runtime, random_lb, lb_period
									);
							model->init()->run();
							delete model;
						}
						break;
				}
			}
		}
	}
	fpmas::finalize();
}
