set(SOURCES
	llvm-browse)

foreach(SOURCE ${SOURCES})
	configure_file(${SOURCE} ${SOURCE})
endforeach(SOURCE)

install(PROGRAMS ${SOURCES}
  DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
