include(FetchContent)
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()
include(GoogleTest)

add_executable(lms_tests
  test_domain.cpp
  test_services.cpp)
target_link_libraries(lms_tests PRIVATE lms_core GTest::gtest_main)
gtest_discover_tests(lms_tests)
