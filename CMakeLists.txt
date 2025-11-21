cmake_minimum_required(VERSION 3.28)

project(playground LANGUAGES CXX)

set(CMAKE_CXX_COMPILER clang++ CACHE STRING "C++ compiler")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
  message(STATUS "System: ${CMAKE_SYSTEM_NAME}")

  if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_link_options(-fuse-ld=lld)
  elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_link_options(-fuse-ld=lld)
  elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") # clang-cl
      add_compile_options(/std:c++20 /EHsc /W4)
    else() # MinGW clang++
      add_compile_options(-Wall -Wextra -Wpedantic)
      add_link_options(-fuse-ld=lld)
    endif()
  endif()
endif()

# stb
include(FetchContent)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

add_executable(digital_clock programs/digital_clock.cpp)
target_include_directories(digital_clock
  PUBLIC ${PROJECT_SOURCE_DIR}/TerminalAPI/include
)

add_executable(rainbow programs/rainbow.cpp)
target_include_directories(rainbow
  PUBLIC ${PROJECT_SOURCE_DIR}/TerminalAPI/include
)

add_executable(checkerboard programs/checkerboard.cpp)
target_include_directories(checkerboard
  PUBLIC ${PROJECT_SOURCE_DIR}/TerminalAPI/include
)

add_executable(img_viewer programs/img_viewer.cpp)
target_include_directories(img_viewer
  PUBLIC ${PROJECT_SOURCE_DIR}/TerminalAPI/include ${stb_SOURCE_DIR}
)

add_executable(binary_dump_bmp programs/binary_dump_bmp.cpp)
target_include_directories(binary_dump_bmp
  PUBLIC ${PROJECT_SOURCE_DIR}/TerminalAPI/include
)


