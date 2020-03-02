set(SOURCES
  Argument.cpp
  BasicBlock.cpp
  Function.cpp
  GlobalAlias.cpp
  GlobalVariable.cpp
  Instruction.cpp
  MDNode.cpp
  Module.cpp
  INavigable.cpp
  Logging.cpp
  Parser.cpp
  SourceRange.cpp
  String.cpp
  StructType.cpp
  Value.cpp)

set(LIB_SYMLINK ${PROJECT_SOURCE_DIR}/llvm_browse/lib${LIB_LLVM_BROWSE_LIB}.so)

add_library(${LIB_LLVM_BROWSE_LIB} SHARED
  ${SOURCES})

link_directories(${LLVM_LIB_DIR})

target_link_libraries(${LIB_LLVM_BROWSE_LIB}
  ${LLVM_LIBS})

set_target_properties(${LIB_LLVM_BROWSE_LIB}
  PROPERTIES 
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/llvm_browse)
