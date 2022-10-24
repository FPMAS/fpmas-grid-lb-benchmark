cmake_minimum_required(VERSION 3.10)

get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(fpmas-metamodel)

find_package(fpmas 1.6 REQUIRED)

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

add_executable(gen-seed gen_seed.cpp)

add_subdirectory(tests)

# Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	set(DOXYGEN_HTML_OUTPUT ".")
	set(DOXYGEN_SORT_MEMBER_DOCS "NO")
	set(DOXYGEN_HAVE_DOT "YES")
	#set(DOXYGEN_TEMPLATE_RELATIONS "YES")
	#set(DOXYGEN_CLANG_ASSISTED_PARSING "YES")
	#set(DOXYGEN_CLANG_DATABASE_PATH "build/compile_commands.json")
	set(DOXYGEN_HIDE_FRIEND_COMPOUNDS "YES")
	set(DOXYGEN_EXCLUDE_SYMBOLS
		[[IMPLEM_CELL_INTERACTIONS]]
		[[IMPLEM_CELL_INTERACTION]]
		[[YAML::*]]
		# No need extra documentation for those specializations
		[[MetaModel< GridModel< SyncMode, MetaGridCell >, MetaGridAgent >]]
		[[MetaModel< SpatialModel< SyncMode, MetaGraphCell >, MetaGraphAgent >]]
		[[MetaAgentSerialization< MetaGridAgent >]]
		[[MetaAgentSerialization< MetaGraphAgent >]]
		[[MetaAgent< SpatialAgent< MetaGraphAgent, MetaGraphCell >, GraphRange< MetaGraphCell > >]]
		[[MetaAgent< GridAgent< MetaGridAgent, MetaGridCell >, MooreRange< MooreGrid< MetaGridCell > > >]]
		)
	doxygen_add_docs(doc
		${CMAKE_CURRENT_SOURCE_DIR}/include
		)
endif()
