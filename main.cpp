#include "fpmas.h"
#include "benchmark.h"

#define GRID_WIDTH 100u
#define GRID_HEIGHT 100u
#define OCCUPATION_RATE 0.5f

FPMAS_JSON_SET_UP(fpmas::model::GridCell::JsonBase, BenchmarkAgent::JsonBase);

int main(int argc, char** argv) {
	FPMAS_REGISTER_AGENT_TYPES(fpmas::model::GridCell::JsonBase, BenchmarkAgent::JsonBase);

	fpmas::init(argc, argv);
	{
		std::array<TestCase*, 3> test_cases;
		std::array<fpmas::scheduler::Scheduler, 3> schedulers;
		std::array<fpmas::runtime::Runtime, 3> runtimes {
			schedulers[0], schedulers[1], schedulers[2]
		};

		fpmas::model::ZoltanLoadBalancing zoltan_lb(fpmas::communication::WORLD);
		TestCase zoltan_lb_test(
			"zoltan_lb", GRID_WIDTH, GRID_HEIGHT, OCCUPATION_RATE,
			schedulers[0], runtimes[0], zoltan_lb
		);
		test_cases[0] = &zoltan_lb_test;

		fpmas::model::ScheduledLoadBalancing scheduled_load_balancing(
				zoltan_lb, schedulers[1], runtimes[1]
				);
		TestCase scheduled_lb_test(
			"scheduled_lb", GRID_WIDTH, GRID_HEIGHT, OCCUPATION_RATE,
			schedulers[1], runtimes[1], scheduled_load_balancing
		);
		test_cases[1] = &scheduled_lb_test;

		fpmas::model::GridLoadBalancing grid_lb(
				GRID_WIDTH, GRID_HEIGHT, fpmas::communication::WORLD
				);
		TestCase grid_lb_test(
			"grid_lb", GRID_WIDTH, GRID_HEIGHT, OCCUPATION_RATE,
			schedulers[2], runtimes[2], grid_lb
		);
		test_cases[2] = &grid_lb_test;

		for(auto test : test_cases)
			test->run();
	}
	fpmas::finalize();
}
