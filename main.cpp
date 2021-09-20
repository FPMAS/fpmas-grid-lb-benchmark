#include "fpmas.h"
#include "benchmark.h"
#include "fpmas/model/spatial/cell_load_balancing.h"

FPMAS_JSON_SET_UP(
		GridCell::JsonBase,
		BenchmarkAgent::JsonBase,
		BenchmarkCell::JsonBase
		);

int main(int argc, char** argv) {
	FPMAS_REGISTER_AGENT_TYPES(
			GridCell::JsonBase,
			BenchmarkAgent::JsonBase,
			BenchmarkCell::JsonBase
			);

	fpmas::init(argc, argv);
	{
		BenchmarkConfig config(argv[1]);

		ZoltanLoadBalancing zoltan_lb(fpmas::communication::WORLD);
		for(auto lb_period : config.test_cases[ZOLTAN_LB]) {
			fpmas::scheduler::Scheduler scheduler;
			fpmas::runtime::Runtime runtime(scheduler);
			TestCase zoltan_lb_test(
					"zoltan_lb", config,
					scheduler, runtime, zoltan_lb, lb_period
					);
			zoltan_lb_test.run();
		}

		for(auto lb_period : config.test_cases[SCHEDULED_LB]) {
			fpmas::scheduler::Scheduler scheduler;
			fpmas::runtime::Runtime runtime(scheduler);
			ScheduledLoadBalancing scheduled_load_balancing(
					zoltan_lb, scheduler, runtime
					);
			TestCase scheduled_lb_test(
					"scheduled_lb", config,
					scheduler, runtime, scheduled_load_balancing, lb_period
					);
			scheduled_lb_test.run();
		}

		for(auto lb_period : config.test_cases[GRID_LB]) {
			fpmas::scheduler::Scheduler scheduler;
			fpmas::runtime::Runtime runtime(scheduler);
			GridLoadBalancing grid_lb(
					config.grid_width, config.grid_height, fpmas::communication::WORLD
					);
			TestCase grid_lb_test(
					"grid_lb", config,
					scheduler, runtime, grid_lb, lb_period
					);
			grid_lb_test.run();
		}

		for(auto lb_period : config.test_cases[ZOLTAN_CELL_LB]) {
			fpmas::scheduler::Scheduler scheduler;
			fpmas::runtime::Runtime runtime(scheduler);
			CellLoadBalancing zoltan_cell_lb(
					fpmas::communication::WORLD, zoltan_lb
					);
			TestCase zoltan_cell_lb_test(
					"zoltan_cell_lb", config,
					scheduler, runtime, zoltan_cell_lb, lb_period
					);
			zoltan_cell_lb_test.run();
		}

		for(auto lb_period : config.test_cases[RANDOM_LB]) {
			fpmas::scheduler::Scheduler scheduler;
			fpmas::runtime::Runtime runtime(scheduler);
			RandomLoadBalancing random_lb(fpmas::communication::WORLD);
			TestCase random_lb_test(
					"random_lb", config,
					scheduler, runtime, random_lb, lb_period
					);
			random_lb_test.run();
		}
	}
	fpmas::finalize();
}
