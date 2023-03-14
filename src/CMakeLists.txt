include_directories(commands)

file(GLOB_RECURSE FILENAMES *.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(phyLS phyLS.cpp ${FILENAMES})
target_link_libraries(phyLS alice mockturtle libabc-pic)
