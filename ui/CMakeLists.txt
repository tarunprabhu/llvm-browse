set(SOURCES
  UI.cpp)

include_directories(${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS} ${LLVM_LIBRARY_DIR})

add_library(${LIB_LLVM_BROWSE_UI} SHARED
  ${SOURCES})

target_link_libraries(${LIB_LLVM_BROWSE_UI}
  ${LLVM_LIBS}
  ${GTKMM_LIBRARIES})
