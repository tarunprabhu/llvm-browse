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
    ${Python_EXECUTABLE} -m site --user-site
    OUTPUT_VARIABLE Python_INSTALL_FULL_SITEDIR
    RESULT_VARIABLE USER_SITE_FOUND
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT USER_SITE_FOUND EQUAL 0)
    message(FATAL_ERROR "Could not get Python user sitepackage dir")
  endif()
  execute_process(COMMAND
    ${Python_EXECUTABLE} -m site --user-base
    OUTPUT_VARIABLE CMAKE_INSTALL_PREFIX
    RESULT_VARIABLE USER_BASE_FOUND
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT USER_BASE_FOUND EQUAL 0)
    message(FATAL_ERROR "Could not get Python user base dir")
  endif()
else()
  set(Python_INSTALL_FULL_SITEDIR ${Python_SITELIB})
endif()

# Python_SOABI is set automatically by later versions of CMake
if(NOT Python_SOABI)
  execute_process(COMMAND
    ${Python_EXECUTABLE}
    -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))"
    OUTPUT_VARIABLE Python_SOABI
    RESULT_VARIABLE SOABI_FOUND
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT SOABI_FOUND EQUAL 0)
    message(FATAL_ERROR "Could not get extension suffix")
  endif()
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

# Configure other required packages.
#
# FIXME?: This currently assumes that if we can find the pkgconfig,
# then the Python bindings will also have been installed. This might not be the
# case but I'm not sure. Probably should find another way of doing this
# just in case

function(check_python_bindings BINDING_NAME BINDING_VERSION)
  # Do not run check if a cache variable was found, typically on a previous
  # invocation of cmake. There might be a better way of doing this that is 
  # not manual
  if(NOT Py${BINDING_NAME}_FOUND)
    message(STATUS "Checking for Python bindings ${BINDING_NAME} ${BINDING_VERSION}")
    execute_process(COMMAND
      ${Python_EXECUTABLE} -c "import gi; gi.require_version('${BINDING_NAME}', '${BINDING_VERSION}')"
      OUTPUT_QUIET
      ERROR_VARIABLE STDERR)
    if(STDERR)
      message(FATAL_ERROR "Missing required Python bindings: ${BINDING_NAME} ${BINDING_VERSION}\n${STDERR}")
    else()
      message(STATUS "  Found '${BINDING_NAME}' '${BINDING_VERSION}'")
      # Set a variable in the parent scope so CMake doesn't run these checks 
      # every time make is called
      set(Py${BINDING_NAME}_FOUND YES CACHE BOOL "Found Python binding for ${BINDING_NAME} ${BINDING_VERSION}")
    endif()
  endif()
endfunction()

# Checks if a Python module is available
# By default, the modules are not required. If a module is required, an 
# additional REQUIRED argument must be passed to the function
function(check_python_module MODULE_NAME)
  if(${ARGC} GREATER 1)
    if(ARGV1 STREQUAL "REQUIRED")
      set(REQUIRED "REQUIRED")
    else()
      message(FATAL_ERROR "INTERNAL ERROR: Value of optional "
        "argument for function check_python_module() must be REQUIRED. "
        "Got ${ARGV1}")
    endif()
  endif()

  if(NOT Py${MODULE_NAME}_FOUND)
    message(STATUS "Checking for Python module '${MODULE_NAME}'")
    execute_process(COMMAND
      ${Python_EXECUTABLE} -c "import ${MODULE_NAME}"
      OUTPUT_QUIET
      ERROR_QUIET
      RESULT_VARIABLE RETCODE)
    if(NOT RETCODE EQUAL 0)
      if(REQUIRED)
        message(FATAL_ERROR "Missing requied Python module: ${MODULE_NAME}")
      else()
        message(STATUS "  MISSING optional Python module ${MODULE_NAME}")
      endif()
    else()
      message(STATUS "  Found Python module ${MODULE_NAME}")
      set(Py${MODULE_NAME}_FOUND YES CACHE BOOL "Found Python module ${MODULE_NAME}")
    endif()
  endif()
endfunction()

# Check if gi python module is installed
check_python_module(gi REQUIRED)

# GObject
set(PY_GOBJECT_VERSION 2.0)
set(GOBJECT_PACKAGE_MINIMUM 2.52)
set(GOBJECT_PACKAGE gobject-${PY_GOBJECT_VERSION}>=${GOBJECT_PACKAGE_MINIMUM})
pkg_check_modules(GOBJECT_REQUIRED ${GOBJECT_PACKAGE})
check_python_bindings(GObject ${PY_GOBJECT_VERSION})

# GLib
set(PY_GLIB_VERSION 2.0)
set(GLIB_PACKAGE_MINIMUM 2.48)
set(GLIB_PACKAGE glib-${PY_GLIB_VERSION}>=${GLIB_PACKAGE_MINIMUM})
pkg_check_modules(GLIB REQUIRED ${GLIB_PACKAGE})
check_python_bindings(GLib ${PY_GLIB_VERSION})

# GIO
set(PY_GIO_VERSION 2.0)
set(GIO_PACKAGE gio-${PY_GIO_VERSION})
pkg_check_modules(GIO REQUIRED ${GIO_PACKAGE}>=${GLIB_PACKAGE_MINIMUM})
check_python_bindings(Gio ${PY_GIO_VERSION})

# GTK+
set(PY_GTK_VERSION 3.0)
set(GTK_PACKAGE_MINIMUM 3.20)
set(GTK_PACKAGE gtk+-${PY_GTK_VERSION}>=${GTK_PACKAGE_MINIMUM})
pkg_check_modules(GTK REQUIRED ${GTK_PACKAGE})
check_python_bindings(Gtk ${PY_GTK_VERSION})

# GDK
set(PY_GDK_VERSION 3.0)
set(GDK_PACKAGE gdk-${PY_GDK_VERSION}>=${GTK_PACKAGE_MINIMUM})
pkg_check_modules(GDK REQUIRED ${GDK_PACKAGE})
check_python_bindings(Gdk ${PY_GDK_VERSION})

# GdkPixbuf
set(PY_GDKPIXBUF_VERSION 2.0)
set(GDKPIXBUF_PACKAGE_MINIMUM 2.36)
set(GDKPIXBUF_PACKAGE gdk-pixbuf-${PY_GDKPIXBUF_VERSION}>=${GDKPIXBUF_PACKAGE_MINIMUM}})
pkg_check_modules(GDK_PIXBUF REQUIRED ${GDKPIXBUF_PACKAGE})
check_python_bindings(GdkPixbuf ${PY_GDKPIXBUF_VERSION})

# GTKSource
set(PY_GTKSOURCE_VERSION 4)
set(GTKSOURCE_PACKAGE_MINIMUM)
set(GTKSOURCE_PACKAGE gtksourceview-${PY_GTKSOURCE_VERSION}>=${GTKSOURCE_PACKAGE_MINIMUM}})
pkg_check_modules(GTK_SOURCE REQUIRED ${GTKSOURCE_PACKAGE})
check_python_bindings(GtkSource ${PY_GTKSOURCE_VERSION})

# Pango
set(PY_PANGO_VERSION 1.0)
set(PANGO_PACKAGE_MINIMUM 1.40)
set(PANGO_PACKAGE pango)
pkg_check_modules(PANGO REQUIRED ${PANGO_PACKAGE}>=${PANGO_PACKAGE_MINIMUM})
check_python_bindings(Pango ${PY_PANGO_VERSION})

# PangoCairo
set(PY_PANGOCAIRO_VERSION 1.0)
set(PANGO_PACKAGE pangocairo)
pkg_check_modules(PANGO_CAIRO REQUIRED ${PANGO_PACKAGE}>=${PANGO_PACKAGE_MINIMUM})
check_python_bindings(PangoCairo ${PY_PANGOCAIRO_VERSION})

# Check for other Python modules
check_python_module(termcolor)

# glib_compile_resources
pkg_get_variable(GLIB_COMPILE_RESOURCES ${GIO_PACKAGE} glib_compile_resources)
message(STATUS "Glib resources compiler: ${GLIB_COMPILE_RESOURCES}")

# This must be added after CMAKE_INSTALL_PREFIX is set
include(GNUInstallDirs)
message(STATUS "Installing binaries: ${CMAKE_INSTALL_FULL_BINDIR}")
message(STATUS "Installing libs: ${CMAKE_INSTALL_FULL_LIBDIR}")
message(STATUS "Installing Python modules: ${Python_INSTALL_FULL_SITEDIR}")

set(PYTHON_MODULES llvm_browse_gtk)

# Crate an uninstall target
file(WRITE ${PROJECT_BINARY_DIR}/tmp/uninstall.sh "
#!/bin/sh

if [ -f ${PROJECT_BINARY_DIR}/install_manifest.txt ]; then
  cat ${PROJECT_BINARY_DIR}/install_manifest.txt | xargs -L 1 rm --force --verbose
  for module in ${PYTHON_MODULES}; do
    rm --recursive --force --verbose ${Python_INSTALL_FULL_SITEDIR}/$module
  done
else
  echo 'Nothing to uninstall'
fi")
file(COPY ${PROJECT_BINARY_DIR}/tmp/uninstall.sh
  DESTINATION ${PROJECT_BINARY_DIR}
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
file(REMOVE_RECURSE ${PROJECT_BINARY_DIR}/tmp)

# Setup global compiler options
# Default cmake flags
set(CMAKE_SKIP_BUILD_RPATH False)
set(CMAKE_BUILD_WITH_INSTALL_RPATH False)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH True)
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR};${LLVM_LIBRARY_DIR})

add_compile_options(-Wall -Wextra -pedantic -Wno-unused-parameter -fPIC)
add_compile_options(
  $<$<COMPILE_LANGUAGE:CXX>:-std=c++14>)
add_definitions(${LLVM_DEFINITIONS})
include_directories(
  ${LLVM_INCLUDE_DIRS})
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

set(LIB_LLVM_BROWSE_LIB LLVMBrowse)

add_subdirectory(bin)
add_subdirectory(lib)
add_subdirectory(llvm_browse_gtk)
add_subdirectory(llvm_browse)
