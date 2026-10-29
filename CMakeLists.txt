cmake_minimum_required(VERSION 3.16)
project(lms VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(LMS_BUILD_TESTS "Build the automated tests" ON)
option(LMS_COVERAGE "Instrument the code for gcov/gcovr coverage" OFF)

find_package(PkgConfig REQUIRED)

# SQLite: use pkg-config first (works the same on Linux and MSYS2 and avoids CMake package-config
# naming warnings), and fall back to CMake's own FindSQLite3 module.
pkg_check_modules(SQLITE3 IMPORTED_TARGET sqlite3)
if(NOT SQLITE3_FOUND)
  find_package(SQLite3 REQUIRED)
endif()
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

# The SQL schema lives in sql/schema.sql and is embedded in the program.
file(READ "${CMAKE_SOURCE_DIR}/sql/schema.sql" LMS_SCHEMA_SQL)
configure_file(src/infrastructure/SchemaSql.cpp.in
               "${CMAKE_BINARY_DIR}/generated/SchemaSql.cpp" @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS sql/schema.sql)

add_library(lms_core STATIC
  src/domain/Types.cpp
  src/domain/User.cpp
  src/domain/UserFactory.cpp
  src/domain/Entities.cpp
  src/domain/TenPercentPerDayPolicy.cpp
  src/services/AuthService.cpp
  src/services/AdminService.cpp
  src/services/AssignmentService.cpp
  src/services/SubmissionService.cpp
  src/services/MarkingService.cpp
  src/services/ModerationService.cpp
  src/services/Validation.cpp
  src/infrastructure/Database.cpp
  src/infrastructure/SqliteUserRepository.cpp
  src/infrastructure/SqliteCourseRepository.cpp
  src/infrastructure/SqliteAssignmentRepository.cpp
  src/infrastructure/SqliteSubmissionRepository.cpp
  src/infrastructure/SqliteMarkRepository.cpp
  src/infrastructure/SystemClock.cpp
  src/infrastructure/SodiumPasswordHasher.cpp
  "${CMAKE_BINARY_DIR}/generated/SchemaSql.cpp")
target_include_directories(lms_core PUBLIC src)
if(SQLITE3_FOUND)
  target_link_libraries(lms_core PUBLIC PkgConfig::SQLITE3 PkgConfig::SODIUM)
else()
  target_link_libraries(lms_core PUBLIC SQLite::SQLite3 PkgConfig::SODIUM)
endif()
target_compile_options(lms_core PRIVATE -Wall -Wextra -Wpedantic -Wshadow)

add_executable(lms src/main.cpp src/ui/ConsoleApp.cpp src/ui/Console.cpp
    src/ui/Terminal.cpp)
target_link_libraries(lms PRIVATE lms_core)
target_compile_options(lms PRIVATE -Wall -Wextra -Wpedantic -Wshadow)

if(LMS_COVERAGE)
  target_compile_options(lms_core PUBLIC --coverage -O0 -g)
  target_link_options(lms_core PUBLIC --coverage)
endif()

if(LMS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

install(TARGETS lms RUNTIME DESTINATION bin)
