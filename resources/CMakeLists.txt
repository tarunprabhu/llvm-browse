set(SOURCES
	ui/llvm-browse.glade
	icons/16x16/llvm-browse.svg
	icons/24x24/llvm-browse.svg
	icons/32x32/llvm-browse.svg
	icons/48x48/llvm-browse.svg
	icons/64x64/llvm-browse.svg
	icons/128x128/llvm-browse.svg)

set(TARGET llvm-browse.gresource)
set(GRESOURCE_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.xml)
set(GRESOURCE_TARGET ${PROJECT_SOURCE_DIR}/llvm_browse/${TARGET})

add_custom_command(
  OUTPUT ${GRESOURCE_TARGET}
  COMMAND ${GLIB_COMPILE_RESOURCES} 
  ARGS --target=${GRESOURCE_TARGET} --sourcedir=${CMAKE_CURRENT_SOURCE_DIR} ${GRESOURCE_CONFIG}
#  BYPRODUCTS ${GRESOURCE_TARGET}
  DEPENDS ${SOURCES} ${GRESOURCE_CONFIG}
  COMMENT "Generating GResources")

add_custom_target(${TARGET} ALL
	DEPENDS ${GRESOURCE_TARGET}) 