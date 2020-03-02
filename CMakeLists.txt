cmake_minimum_required(VERSION 3.4)

project(llvm-browse VERSION ${PROJECT_VERSION})

enable_language(C)
enable_language(CXX)

include(GNUInstallDirs)
find_package(PkgConfig)

# Default cmake flags
set(CMAKE_SKIP_BUILD_RPATH False)
set(CMAKE_BUILD_WITH_INSTALL_RPATH True)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH True)

#
# Configure LLVM
#
# -DLLVM_INSTALLED=<path>
#    to use LLVM installed to a non-standard directory
#
# -DLLVM_LINK_LIBLLVM=<bool>
#    to use libLLVM if it is found
#
# -DLLVM_LINK_SHARED=<bool>
#    to link against LLVM's dynamic libraries instead of static libraries
# 
set(LLVM_MINIMUM_VERSION 8.0.0)
if(LLVM_INSTALLED)
  message(STATUS "Looking for LLVM in ${LLVM_INSTALLED}")
  find_package(LLVM REQUIRED CONFIG NO_DEFAULT_PATH
    PATHS ${LLVM_INSTALLED})
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
if(LLVM_LINK_SHARED OR LLVM_LINK_LIBLLVM)
  if(LLVM_LINK_LIBLLVM)
    # Look for a shared library named libLLVM. At some point, this should be
    # fixed, but there's doesn't seem to be a consistent way of finding it
    # across LLVM versions
    find_library(LLVM_LIBS
      NAMES LLVM LLVM-${LLVM_PACKAGE_VERSION} LLVM-${LLVM_VERSION_MAJOR}
      NAMES_PER_DIR
      PATHS ${LLVM_LIBRARY_DIR}
      NO_DEFAULT_PATH)
    # If we couldn't find libLLVM, then use llvm-config to find the
    # individual shared library to link against.
    if(LLVM_LIBS STREQUAL "LLVM_LIBS-NOTFOUND")
      unset(LLVM_LIBS)
    endif()
  endif()
  if(NOT LLVM_LIBS)
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

# Configure Gio. We only really need this to get glib-compile-resources
#
set(GIO_PACKAGE gio-2.0)
pkg_check_modules(GIO REQUIRED QUIET ${GIO_PACKAGE})
pkg_get_variable(GLIB_COMPILE_RESOURCES ${GIO_PACKAGE} glib_compile_resources)
message(STATUS "Glib resources compiler: ${GLIB_COMPILE_RESOURCES}")

# Setup global compiler options
add_compile_options(-Wall -Wextra -Wno-unused-parameter -fPIC)
add_compile_options(
  $<$<COMPILE_LANGUAGE:CXX>:-std=c++14>)
add_definitions(${LLVM_DEFINITIONS})
include_directories(
  ${LLVM_INCLUDE_DIRS})
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

set(LIB_LLVM_BROWSE_LIB LLVMBrowse)

add_subdirectory(lib)
add_subdirectory(resources)
