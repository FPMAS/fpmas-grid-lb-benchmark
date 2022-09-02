#include "metamodel.h"

MetaModelFactory::MetaModelFactory(Environment environment, SyncMode sync_mode)
	: environment(environment), sync_mode(sync_mode) {
	}

#define BUILD_MODEL(MODEL, SYNCHRO)\
	return new MODEL<fpmas::synchro::SYNCHRO>(\
			lb_algorithm_name, config, scheduler, runtime,\
			lb_algorithm, lb_period\
			);

#define SWITCH_SYNC_MODES(MODEL)\
	switch(sync_mode) {\
		case SyncMode::GHOST_MODE:\
			BUILD_MODEL(MODEL, GhostMode);\
		case SyncMode::GLOBAL_GHOST_MODE:\
			BUILD_MODEL(MODEL, GlobalGhostMode);\
		case SyncMode::HARD_SYNC_MODE:\
			BUILD_MODEL(MODEL, HardSyncMode);\
		default:\
			return nullptr;\
	}

BasicMetaModel* MetaModelFactory::build(
		std::string lb_algorithm_name, BenchmarkConfig config,
		fpmas::api::scheduler::Scheduler& scheduler,
		fpmas::api::runtime::Runtime& runtime,
		fpmas::api::model::LoadBalancing& lb_algorithm,
		fpmas::scheduler::TimeStep lb_period
		) {
	switch(environment) {
		case Environment::GRID:
			SWITCH_SYNC_MODES(MetaGridModel);
		default:
			// All other graph types
			SWITCH_SYNC_MODES(MetaGraphModel);
	}
}

