set(SOURCES
  App.cpp
  main.cpp)

add_executable(${EXECUTABLE}
  ${SOURCES})

target_link_libraries(${EXECUTABLE}
  ${LIB_LLVM_BROWSE_LIB} ${LIB_LLVM_BROWSE_UI})
