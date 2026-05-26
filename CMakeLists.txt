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

# Box2D (physics)
FetchContent_Declare(
    box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG        v3.1.0
)
FetchContent_MakeAvailable(box2d)

# Box2D (especially in Release) can trigger -Wmaybe-uninitialized inside its C sources
# (e.g. distance.c). This becomes fatal when the user has -Werror in their environment.
# We only suppress it for compilers that actually understand the flag.
if (TARGET box2d)
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(box2d PRIVATE -Wno-maybe-uninitialized)
  elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Clang does not have -Wno-maybe-uninitialized in the same form.
    # The equivalent is usually not needed, but we can silence the generic one if it appears.
    # target_compile_options(box2d PRIVATE -Wno-uninitialized)
  endif()
endif()

# programs
add_executable(digital_clock programs/digital_clock.cpp)
target_include_directories(digital_clock
  PUBLIC ${PROJECT_SOURCE_DIR}/include
)

add_executable(checkerboard programs/checkerboard.cpp)
target_include_directories(checkerboard
  PUBLIC ${PROJECT_SOURCE_DIR}/include
)

add_executable(img_viewer programs/img_viewer.cpp)
target_include_directories(img_viewer
  PUBLIC ${PROJECT_SOURCE_DIR}/include ${stb_SOURCE_DIR}
)

add_executable(binary_dump_bmp programs/binary_dump_bmp.cpp)
target_include_directories(binary_dump_bmp
  PUBLIC ${PROJECT_SOURCE_DIR}/include
)

add_executable(random_dist programs/random_dist.cpp)
target_include_directories(random_dist
  PUBLIC ${PROJECT_SOURCE_DIR}/include
)

add_executable(bounce programs/bounce.cpp)
target_include_directories(bounce
  PUBLIC ${PROJECT_SOURCE_DIR}/include
)

add_executable(fire programs/fire.cpp)
target_include_directories(fire
  PUBLIC ${PROJECT_SOURCE_DIR}/include
)

add_executable(physics programs/physics.cpp)
target_include_directories(physics
  PUBLIC ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(physics PRIVATE box2d)

add_executable(demolition programs/demolition.cpp)
target_include_directories(demolition
  PUBLIC ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(demolition PRIVATE box2d)

# experimental
add_executable(matcher experimental/Matcher.cpp)

