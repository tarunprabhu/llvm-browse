set(SOURCES
  Argument.cpp
  BasicBlock.cpp
  Constant.cpp
  Function.cpp
  GlobalVariable.cpp
  Instruction.cpp
  Module.cpp
  Navigable.cpp
  Parser.cpp
  SourceFile.cpp
  SourceRange.cpp
  String.cpp
  StructType.cpp
  Value.cpp)

add_library(${LIB_LLVM_BROWSE_LIB} SHARED
  ${SOURCES})

link_directories(${LLVM_LIB_DIR})

target_link_libraries(${LIB_LLVM_BROWSE_LIB}
  ${LLVM_LIBS})
