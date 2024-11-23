project(gol-sat)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++11 -O2")

include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(gol-sat
  src/commandline.cpp
  src/field.cpp
  src/formula.cpp
  src/main.cpp
  src/pattern.cpp
)

include(Mergesat)
include_directories(${Mergesat_INCLUDE_DIRS})
target_link_libraries(gol-sat ${Mergesat_LIBRARIES})
add_dependencies(gol-sat MergesatLib)

# Add pthread explicitly
target_link_libraries(gol-sat PRIVATE pthread)
