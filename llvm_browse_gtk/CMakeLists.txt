set(SOURCES
	__init__.py
	__main__.py
	application.py
	alias.py
	argument.py
	block.py
	entity.py
	function.py
	global_variable.py
	metadata.py
	module.py
	options.py
	struct.py
	ui.py)

set(RESOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/llvm-browse.glade
	${PROJECT_SOURCE_DIR}/icons/16x16/llvm-browse.svg
	${PROJECT_SOURCE_DIR}/icons/24x24/llvm-browse.svg
	${PROJECT_SOURCE_DIR}/icons/32x32/llvm-browse.svg
	${PROJECT_SOURCE_DIR}/icons/48x48/llvm-browse.svg
	${PROJECT_SOURCE_DIR}/icons/64x64/llvm-browse.svg
	${PROJECT_SOURCE_DIR}/icons/128x128/llvm-browse.svg)

foreach(PY_FILE ${SOURCES})
	configure_file(${PY_FILE} ${PY_FILE})
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PY_FILE}
		DESTINATION ${Python_INSTALL_FULL_SITEDIR}/llvm_browse_gtk)
endforeach(PY_FILE)

set(TARGET llvm-browse.gresource)
set(GRESOURCE_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.xml)
set(GRESOURCE_TARGET ${CMAKE_CURRENT_BINARY_DIR}/${TARGET})

add_custom_command(
  OUTPUT ${GRESOURCE_TARGET}
  COMMAND ${GLIB_COMPILE_RESOURCES} 
  ARGS --target=${GRESOURCE_TARGET} --sourcedir=${PROJECT_SOURCE_DIR} ${GRESOURCE_CONFIG}
  DEPENDS ${RESOURCES} ${GRESOURCE_CONFIG}
  COMMENT "Generating GResources")

add_custom_target(${TARGET} ALL
	DEPENDS ${GRESOURCE_TARGET}) 

install(FILES ${GRESOURCE_TARGET}
	DESTINATION ${Python_INSTALL_FULL_SITEDIR}/llvm_browse_gtk)
