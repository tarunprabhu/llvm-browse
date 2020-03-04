set(SOURCES
	llvm-browse)

foreach(SOURCE ${SOURCES})
	configure_file(${SOURCE} ${SOURCE})
endforeach(SOURCE)

install(PROGRAMS ${SOURCES}
  DESTINATION ${PROJECT_INSTALL_BIN_DIR})
