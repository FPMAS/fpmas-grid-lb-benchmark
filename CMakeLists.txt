cmake_minimum_required(VERSION 3.10)

get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${DIR_NAME})

find_package(fpmas 1.5 REQUIRED)

include(FetchContent)
FetchContent_Declare(
	yaml-cpp
	GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
	GIT_TAG yaml-cpp-0.7.0
	)
FetchContent_Declare(
	cli11
	GIT_REPOSITORY https://github.com/CLIUtils/CLI11
	GIT_TAG        v2.2.0
	)

FetchContent_MakeAvailable(yaml-cpp cli11)

add_library(fpmas-metamodel-lib
	src/agent.cpp
	src/cell.cpp
	src/interactions.cpp
	src/probe.cpp
	src/metamodel.cpp
	src/config.cpp
	src/output.cpp
	src/dot.cpp)
include_directories(include)
target_link_libraries(fpmas-metamodel-lib fpmas::fpmas yaml-cpp::yaml-cpp
	CLI11::CLI11)

add_executable(fpmas-metamodel run_model.cpp)
target_link_libraries(fpmas-metamodel fpmas-metamodel-lib)

add_executable(fpmas-metamodel-graph-stats graph_stats.cpp)
target_link_libraries(fpmas-metamodel-graph-stats fpmas-metamodel-lib)

add_subdirectory(tests)
