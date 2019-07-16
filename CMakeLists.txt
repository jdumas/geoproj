cmake_minimum_required(VERSION 3.11)
project(geoproj)

# sanitizers
option(SANITIZE_ADDRESS   "Sanitize Address"        OFF)
option(SANITIZE_MEMORY    "Sanitize Memory"         OFF)
option(SANITIZE_THREAD    "Sanitize Thread"         ON)
option(SANITIZE_UNDEFINED "Sanitize Undefined"      OFF)

# uncomment the following lines to force options which are already cached
# set(SANITIZE_ADDRESS           ON   CACHE BOOL "" FORCE)
# set(SANITIZE_MEMORY            OFF  CACHE BOOL "" FORCE)
# set(SANITIZE_THREAD            ON  CACHE BOOL "" FORCE)
# set(SANITIZE_UNDEFINED         ON   CACHE BOOL "" FORCE)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(External)
include(UseColors)

gazebo_import(geogram tbb cli11 sanitizers)

add_executable(voronoi
	src/main.cpp
	src/voronoi.cpp
	src/voronoi.h
)

target_link_libraries(voronoi PUBLIC
	CLI11::CLI11
	tbb::tbb
	geogram::geogram
)

target_compile_features(voronoi PUBLIC cxx_std_17)
target_compile_definitions(voronoi PUBLIC "-DDATA_FOLDER=\"${DATA_FOLDER}\"")

add_sanitizers(voronoi)
