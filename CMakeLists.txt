cmake_minimum_required(VERSION 3.14)

project(llvm-browse VERSION 0.1)

enable_language(CXX)

include(GNUInstallDirs)
find_package(LLVM REQUIRED CONFIG)
find_package(wxWidgets REQUIRED COMPONENTS core base)

# We need all of LLVM to be compiled into a single shared object. But the name
# of this object apparently need not always be libLLVM.so, so we need to look
# around a bit to get the right module
find_library(LLVM_LIB_SHARED
  NAMES "LLVM" "LLVM-${LLVM_VERSION}"
  PATHS ${LLVM_LIBRARY_DIRS}
  NO_DEFAULT_PATH)
if("${LLVM_LIB_SHARED}" STREQUAL "CMAKE_LLVM_SHARED-NOTFOUND")
  message(FATAL_ERROR "Could not find LLVM shared object (libLLVM.so)")
endif()

add_compile_options(-Wall -Wextra -Wno-unused-parameter -fPIC)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++14>)
add_definitions(${LLVM_DEFINITIONS})
include_directories(BEFORE ${LLVM_INCLUDE_DIRS})

set(EXECUTABLE llvm-browse)
set(LIB_LLVM_BROWSE_LIB LLVMBrowse)
# set(LIB_LLVM_BROWSE_UI libLLVMBrowseUI)

add_subdirectory(lib)
# add_subdirectory(ui)

set(SOURCES llvm-browse.cpp)
add_executable(${EXECUTABLE}
  ${SOURCES})
target_link_libraries(${EXECUTABLE}
  ${LIB_LLVM_BROWSE_LIB} ${LIB_LLVM_BROWSE_UI})
