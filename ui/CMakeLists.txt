set(SOURCES
  )

add_library(${LIB_LLVM_BROWSE_UI})

target_link_libraries(${LIB_LLVM_BROWSE_UI}
  wxWidgets_LIBRARIES)
