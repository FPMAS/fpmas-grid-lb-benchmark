#include "config.h"

BenchmarkConfig::BenchmarkConfig(std::string config_file) {
	YAML::Node config = YAML::LoadFile(config_file);
	grid_width = config["grid_width"].as<std::size_t>();
	grid_height = config["grid_height"].as<std::size_t>();
	occupation_rate = config["occupation_rate"].as<float>();
	num_steps = config["num_steps"].as<fpmas::api::scheduler::TimeStep>();
	utility = config["cell_distribution"].as<Utility>();
	attractors = config["attractors"].as<std::vector<Attractor>>();
	auto test_cases_vec = config["test_cases"].as<std::vector<TestCaseConfig>>();
	for(auto item : test_cases_vec)
		test_cases[item.algorithm] = item.lb_periods;
}

namespace YAML {
	Node convert<Utility>::encode(const Utility& utility) {
		switch(utility) {
			case UNIFORM:
				return Node("UNIFORM");
			case LINEAR:
				return Node("LINEAR");
			case INVERSE:
				return Node("INVERSE");
			case STEP:
				return Node("STEP");
			default:
				return Node();
		}
	}

	bool convert<Utility>::decode(const Node &node, Utility& utility) {
		std::string str = node.as<std::string>();
		if(str == "UNIFORM") {
			utility = UNIFORM;
			return true;
		}
		if(str == "LINEAR") {
			utility = LINEAR;
			return true;
		}
		if(str == "INVERSE") {
			utility = INVERSE;
			return true;
		}
		if(str == "STEP") {
			utility = STEP;
			return true;
		}
		return false;
	}

	Node convert<LbAlgorithm>::encode(const LbAlgorithm& lb_algorithm) {
		switch(lb_algorithm) {
			case SCHEDULED_LB:
				return Node("SCHEDULED_LB");
			case ZOLTAN_LB:
				return Node("ZOLTAN_LB");
			case GRID_LB:
				return Node("GRID_LB");
			case ZOLTAN_CELL_LB:
				return Node("ZOLTAN_CELL_LB");
			case RANDOM_LB:
				return Node("RANDOM_LB");
			default:
				return Node();
		}
	}

	bool convert<LbAlgorithm>::decode(const Node &node, LbAlgorithm& lb_algorithm) {
		std::string str = node.as<std::string>();
		if(str == "SCHEDULED_LB") {
			lb_algorithm = SCHEDULED_LB;
			return true;
		}
		if(str == "ZOLTAN_LB") {
			lb_algorithm = ZOLTAN_LB;
			return true;
		}
		if(str == "GRID_LB") {
			lb_algorithm = GRID_LB;
			return true;
		}
		if(str == "ZOLTAN_CELL_LB") {
			lb_algorithm = ZOLTAN_CELL_LB;
			return true;
		}
		if(str == "RANDOM_LB") {
			lb_algorithm = RANDOM_LB;
			return true;
		}
		return false;
	}

	Node convert<Attractor>::encode(const Attractor& attractor) {
		Node point;
		point.push_back(attractor.center.x);
		point.push_back(attractor.center.y);

		Node node;
		node.push_back(point);
		node.push_back(attractor.radius);
		return node;
	}

	bool convert<Attractor>::decode(const Node &node, Attractor& attractor) {
		// The root node contains 2 elements
		if(!node.IsSequence() || node.size() != 2)
			return false;
		// The first is a point
		if(!node[0].IsSequence() || node.size() != 2)
			return false;
		// The second is the radius
		if(!node[1].IsScalar())
			return false;

		attractor.center = {
			node[0][0].as<fpmas::api::model::DiscreteCoordinate>(),
			node[0][1].as<fpmas::api::model::DiscreteCoordinate>(),
		};
		attractor.radius = node[1].as<float>();

		return true;
	}

	Node convert<TestCaseConfig>::encode(const TestCaseConfig& test_case_config) {
		Node node;
		node.push_back(test_case_config.algorithm);
		node.push_back(test_case_config.lb_periods);
		return node;
	}

	bool convert<TestCaseConfig>::decode(const Node &node, TestCaseConfig& test_case_config) {
		if(!node.IsSequence() || node.size() != 2)
			return false;
		test_case_config.algorithm = node[0].as<LbAlgorithm>();
		test_case_config.lb_periods = node[1].as<std::vector<fpmas::api::scheduler::TimeStep>>();
		return true;
	}
}
