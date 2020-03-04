cmake_minimum_required(VERSION 3.4)

project(llvm-browse VERSION 0.1)

enable_language(C)
enable_language(CXX)

find_package(PkgConfig)

#
# Configure Python
#
# TODO: Allow for an alternative Python installation maybe?
#  
#  -DINSTALL_AS_USER=<bool>
#     To install without superuser permissions
#
set(PYTHON_MINIMUM_VERSION 3.6.0)
find_package(Python COMPONENTS Interpreter Development)
if(NOT Python_Development_FOUND)
  message(FATAL_ERROR "Could not find Python development libraries")
endif()
if(NOT Python_Interpreter_FOUND)
  message(FATAL_ERROR "Could not find Python interpreter")
endif()
if(Python_VERSION VERSION_LESS "${PYTHON_MINIMUM_VERSION}")
  message(FATAL_ERROR 
    "Python minimum version required: ${PYTHON_MINIMUM_VERSION}")
endif()

if(INSTALL_AS_USER)
  execute_process(COMMAND 
    ${Python_EXECUTABLE} -c "import site; print(site.USER_SITE)"
    OUTPUT_VARIABLE PROJECT_INSTALL_LIB_DIR
    RESULT_VARIABLE USER_SITE_FOUND
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT USER_SITE_FOUND EQUAL 0)
    message(FATAL_ERROR "Could not get Python sitepackage dir")
  endif()
  execute_process(COMMAND
    ${Python_EXECUTABLE} 
    -c "import sysconfig; print(sysconfig.get_paths('posix_user')['scripts'])"
    OUTPUT_VARIABLE PROJECT_INSTALL_BIN_DIR
    RESULT_VARIABLE SCRIPTS_DIR_FOUND
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT SCRIPTS_DIR_FOUND EQUAL 0)
    message(FATAL_ERROR "Could not get Python bin dir")
  endif()
else()
  set(PROJECT_INSTALL_LIB_DIR ${Python_SITELIB})
  execute_process(COMMAND
    ${Python_EXECUTABLE} 
    -c "import sysconfig; print(sysconfig.get_paths()['scripts'])"
    OUTPUT_VARIABLE PROJECT_INSTALL_BIN_DIR
    RESULT_VARIABLE SCRIPTS_DIR_FOUND
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT SCRIPTS_DIR_FOUND EQUAL 0)
    message(FATAL_ERROR "Could not get Python bin dir")
  endif()
endif()

message(STATUS "Python package install directory: ${Python_INSTALL_DIR}")

execute_process(COMMAND
  ${Python_EXECUTABLE} 
  -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))"
  OUTPUT_VARIABLE Python_SOABI
  RESULT_VARIABLE SOABI_FOUND
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT SOABI_FOUND EQUAL 0)
  message(FATAL_ERROR "Could not get extension suffix")
endif()

message(STATUS "Python extension suffix: ${Python_SOABI}")

#
# Configure LLVM
#
# -DLLVM_INSTALLED=<path>
#    to use LLVM installed to a non-standard directory
#
# -DLLVM_LINK_DYNLIB=<bool>
#    to use libLLVM if it is found
#
# -DLLVM_LINK_SHARED=<bool>
#    to link against LLVM's dynamic libraries instead of static libraries
# 
set(LLVM_MINIMUM_VERSION 8.0.0)
if(LLVM_DIR)
  message(STATUS "Looking for LLVM in ${LLVM_DIR}")
  find_package(LLVM REQUIRED CONFIG NO_DEFAULT_PATH
    PATHS ${LLVM_DIR})
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
if(LLVM_LINK_SHARED OR LLVM_LINK_DYNLIB)
  if(LLVM_LINK_DYNLIB)
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

# Configure other required pcakages. 
# 
# FIXME?: This currently assumes that if we can find the pkgconfig, 
# then the Python bindings will also have been installed. This might not be the 
# case but I'm not sure. Probably should find another way of doing this 
# just in case
set(PY_GOBJECT_VERSION 2.0)
set(GOBJECT_PACKAGE gobject-${PY_GOBJECT_VERSION}>=2.52)
pkg_check_modules(GOBJECT_REQUIRED ${GOBJECT_PACKAGE})

set(PY_GLIB_VERSION 2.0)
set(GLIB_PACKAGE glib-${PY_GLIB_VERSION}>=2.48)
pkg_check_modules(GLIB REQUIRED ${GLIB_PACKAGE})

set(PY_GTK_VERSION 3.0)
set(GTK_PACKAGE gtk+-${PY_GTK_VERSION}>=3.20)
pkg_check_modules(GTK REQUIRED ${GTK_PACKAGE})

set(PY_GDK_VERSION 3.0)
set(GDK_PACKAGE gdk-${PY_GDK_VERSION}>=3.20)
pkg_check_modules(GDK REQUIRED ${GDK_PACKAGE})

set(PY_GTKSOURCE_VERSION 4)
set(GTKSOURCE_PACKAGE gtksourceview-${PY_GTKSOURCE_VERSION}>=4)
pkg_check_modules(GTK_SOURCE REQUIRED ${GTKSOURCE_PACKAGE})

set(PY_PANGO_VERSION 1.0)
set(PANGO_PACKAGE pango)
pkg_check_modules(PANGO REQUIRED ${PANGO_PACKAGE}>=1.40)

set(PY_GDKPIXBUF_VERSION 2.0)
set(GDKPIXBUF_PACKAGE gdk-pixbuf-${PY_GDKPIXBUF_VERSION})
pkg_check_modules(GDK_PIXBUF REQUIRED ${GDKPIXBUF_PACKAGE})

# Configure Gio. We only really need this to get glib-compile-resources
#
set(PY_GIO_VERSION 2.0)
set(GIO_PACKAGE gio-${PY_GIO_VERSION})
pkg_check_modules(GIO REQUIRED ${GIO_PACKAGE}>=2.48)
pkg_get_variable(GLIB_COMPILE_RESOURCES ${GIO_PACKAGE} glib_compile_resources)
message(STATUS "Glib resources compiler: ${GLIB_COMPILE_RESOURCES}")

# Setup global compiler options
# Default cmake flags
set(CMAKE_SKIP_BUILD_RPATH False)
set(CMAKE_BUILD_WITH_INSTALL_RPATH True)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH True)
set(CMAKE_INSTALL_RPATH ${Python_INSTALL_DIR}/llvm_browse)

add_compile_options(-Wall -Wextra -Wno-unused-parameter -fPIC)
add_compile_options(
  $<$<COMPILE_LANGUAGE:CXX>:-std=c++14>)
add_definitions(${LLVM_DEFINITIONS})
include_directories(
  ${LLVM_INCLUDE_DIRS})
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

set(LIB_LLVM_BROWSE_LIB LLVMBrowse)

add_subdirectory(llvm_browse_gtk)
add_subdirectory(lib)
add_subdirectory(resources)
add_subdirectory(llvm_browse)

install(PROGRAMS bin/llvm-browse 
  DESTINATION ${PROJECT_INSTALL_BIN_DIR})