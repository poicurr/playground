cmake_minimum_required(VERSION 3.20)

add_executable(digital_clock programs/digital_clock.cpp)
target_include_directories(digital_clock
  PUBLIC ${PROJECT_SOURCE_DIR}/TerminalAPI/include
)

add_executable(rainbow programs/rainbow.cpp)
target_include_directories(rainbow
  PUBLIC ${PROJECT_SOURCE_DIR}/TerminalAPI/include
)

add_executable(bmp_viewer programs/bmp_viewer.cpp)
target_include_directories(bmp_viewer
  PUBLIC ${PROJECT_SOURCE_DIR}/TerminalAPI/include
)

add_executable(binary_viewer programs/binary_viewer.cpp)
target_include_directories(binary_viewer
  PUBLIC ${PROJECT_SOURCE_DIR}/TerminalAPI/include
)


