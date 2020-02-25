cmake_minimum_required(VERSION 3.4)

project(llvm-browse VERSION 0.1)

enable_language(CXX)

include(GNUInstallDirs)
find_package(PkgConfig)


pkg_check_modules(GTKMM gtkmm-3.0 REQUIRED)

#
# Configure LLVM
#
# -DLLVM_DIR=<path>
#    to use LLVM installed to a non-standard directory
#
# -DLLVM_USE_SHARED=<bool>
#    to link against LLVM's dynamic libraries instead of static libraries
# 
set(LLVM_MINIMUM_VERSION "8.0.0")
if(LLVM_DIR)
  find_package(LLVM REQUIRED CONFIG NO_DEFAULT_PATH
    PATHS "${LLVM_DIR}")
else()
  find_package(LLVM REQUIRED CONFIG)
endif()
message(STATUS "LLVM version: ${LLVM_PACKAGE_VERSION}")
if("${LLVM_PACKAGE_VERSION}" VERSION_LESS "${LLVM_MINIMUM_VERSION}")
  message(FATAL_ERROR "Require minimum LLVM version ${LLVM_MINIMUM_VERSION}")
endif()
set(LLVM_REQUIRED_COMPONENTS core irreader support)

# The default is to statically link the LLVM libraries, but during development
# it is much faster to link to the shared library.
if(LLVM_USE_SHARED)
  # Look for a shared library named libLLVM. At some point, this should be
  # fixed, but there's doesn't seem to be a consistent way of finding it
  # across LLVM versions
  find_library(LLVM_LIBS
    NAMES LLVM LLVM-${LLVM_PACKAGE_VERSION} LLVM-${LLVM_VERSION_MAJOR}
    PATHS ${LLVM_LIBRARY_DIR}
    NO_DEFAULT_PATH)
  if(LLVM_LIBS STREQUAL "LLVM_LIBS-NOTFOUND")
    # If we couldn't find libLLVM, then use llvm-config to find the
    # individual shared library to link against
    execute_process(
      COMMAND ${LLVM_TOOLS_BINARY_DIR}/llvm-config
      --link-shared --libfiles ${LLVM_REQUIRED_COMPONENTS}
      OUTPUT_VARIABLE LLVM_LIBS
      ERROR_QUIET
      RESULT_VARIABLE LLVM_LIBS_FOUND
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE)
    if(NOT LLVM_LIBS_FOUND EQUAL 0)
      set(LLVM_LIBS "LLVM_LIBS-NOTFOUND")
    endif()
  endif()
else()
  llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_REQUIRED_COMPONENTS})
endif()
if(LLVM_LIBS STREQUAL "LLVM_LIBS-NOTFOUND")
  message(FATAL_ERROR "Could not find LLVM libraries to link against")
else()
  message(STATUS "LLVM libraries: ${LLVM_LIBS}")
endif()

# Setup global compiler options
add_compile_options(-Wall -Wextra -Wno-unused-parameter -fPIC -std=c++14)
add_definitions(${LLVM_DEFINITIONS})
include_directories(BEFORE ${LLVM_INCLUDE_DIRS} ${GTKMM_INCLUDE_DIRS})

# Top-level binaries and executables to be built
set(EXECUTABLE llvm-browse)
set(LIB_LLVM_BROWSE_LIB LLVMBrowse)
set(LIB_LLVM_BROWSE_UI libLLVMBrowseUI)

add_subdirectory(lib)
add_subdirectory(ui)

# FIXME: This should probaby be moved to a /src directory but I'm not sure if
# anything else will go in there in that case
set(SOURCES llvm-browse.cpp)
add_executable(${EXECUTABLE}
  ${SOURCES})
target_link_libraries(${EXECUTABLE}
  ${LIB_LLVM_BROWSE_LIB} ${LIB_LLVM_BROWSE_UI})
