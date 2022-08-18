FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

add_executable(fpmas-metamodel-tests
	main.cpp
	agent.cpp)

target_link_libraries(fpmas-metamodel-tests
	fpmas-metamodel-lib GTest::gtest_main GTest::gmock_main)
include(GoogleTest)
