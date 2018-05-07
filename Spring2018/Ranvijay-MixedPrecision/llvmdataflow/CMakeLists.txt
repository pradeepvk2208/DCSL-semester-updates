cmake_minimum_required(VERSION 3.8)

project(Dataflow2)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

###################################################################

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

execute_process(COMMAND llvm-config --includedir OUTPUT_VARIABLE LLVM_INCLUDE OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --libs OUTPUT_VARIABLE LLVM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --system-libs OUTPUT_VARIABLE LLVM_SYS_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --libdir OUTPUT_VARIABLE LLVM_LIB_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
# execute_process(COMMAND llvm-config --libnames OUTPUT_VARIABLE LLVM_LIB_NAMES OUTPUT_STRIP_TRAILING_WHITESPACE)
# string(REPLACE " " ";" LLVM_LIB_NAMES ${LLVM_LIB_NAMES})
execute_process(COMMAND llvm-config --cxxflags OUTPUT_VARIABLE LLVM_CXX_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --ldflags OUTPUT_VARIABLE LLVM_LD_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_CXX_FLAGS "${LLVM_CXX_FLAGS} -stdlib=libc++")

# message(STATUS "VAR: \"" ${LLVM_LIB_DIR} "\"")

include_directories(SYSTEM ${LLVM_INCLUDE})
link_directories(${LLVM_LIB_DIR})

add_library(STAModel STAModel.cpp)
target_link_libraries(STAModel ${LLVM_LIBS} ${LLVM_SYS_LIBS})

add_library(model MODULE model.cpp)
# target_link_libraries(model ${LLVM_LIBS} ${LLVM_SYS_LIBS} -ljsoncpp)
target_link_libraries(model "${LLVM_LD_FLAGS} -undefined dynamic_lookup")   # WHY? OS X only?

add_executable(STAscore demo_score.cpp)
target_link_libraries(STAscore STAModel)

add_executable(STAerror demo_error.cpp)
target_link_libraries(STAerror STAModel)

add_executable(STAchildren get_children.cpp)
target_link_libraries(STAchildren STAModel)