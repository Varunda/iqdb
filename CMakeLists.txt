cmake_minimum_required(VERSION 3.14)

project(iqdb)

# Require C++17
# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD_REQUIRED.html
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Disable GNU-specific C++-17 features.
# https://cmake.org/cmake/help/latest/prop_tgt/CXX_EXTENSIONS.html#prop_tgt:CXX_EXTENSIONS
# https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Extensions.html
set(CMAKE_CXX_EXTENSIONS OFF)

# https://cmake.org/cmake/help/latest/variable/CMAKE_EXPORT_COMPILE_COMMANDS.html
# https://clang.llvm.org/docs/JSONCompilationDatabase.html#supported-systems
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# https://stackoverflow.com/questions/1620918/cmake-and-libpthread
set(THREADS_PREFER_PTHREAD_FLAG ON)

# Compile httplib instead of using header-only version.
# https://github.com/yhirose/cpp-httplib/blob/master/CMakeLists.txt
set(HTTPLIB_COMPILE ON)

# Compile json library instead of using header-only version.
# https://github.com/nlohmann/json/blob/develop/CMakeLists.txt#L35
# https://github.com/nlohmann/json#integration
set(JSON_MultipleHeaders ON)

include(FetchContent)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
  GIT_TAG        v0.15.3
)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
  GIT_TAG v3.9.1
)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.6
)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        7.1.3
)

FetchContent_Declare(
  sqliteOrm
  GIT_REPOSITORY https://github.com/fnc12/sqlite_orm
  GIT_TAG        1.6
)

FetchContent_Declare(
  backwardcpp
  GIT_REPOSITORY https://github.com/bombela/backward-cpp
  GIT_TAG        v1.6
)

FetchContent_MakeAvailable(httplib)
FetchContent_MakeAvailable(json)
FetchContent_MakeAvailable(Catch2)
FetchContent_MakeAvailable(fmt)
FetchContent_MakeAvailable(sqliteOrm)
FetchContent_MakeAvailable(backwardcpp)

find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GDLIB REQUIRED gdlib)

add_subdirectory(src)
