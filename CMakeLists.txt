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
FetchContent_MakeAvailable(yaml-cpp)

add_library(fpmas-metamodel-lib
	src/agent.cpp
	src/cell.cpp
	src/probe.cpp
	src/metamodel.cpp
	src/config.cpp
	src/output.cpp
	src/dot.cpp)
include_directories(include)

add_executable(fpmas-metamodel main.cpp)

target_link_libraries(fpmas-metamodel-lib fpmas::fpmas yaml-cpp::yaml-cpp)
target_link_libraries(fpmas-metamodel fpmas-metamodel-lib)

add_subdirectory(tests)
