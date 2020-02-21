set(SOURCES
  Argument.cpp
  BasicBlock.cpp
  Constant.cpp
  Function.cpp
  GlobalVariable.cpp
  Instruction.cpp
  Module.cpp
  SourceFile.cpp
  SourceRange.cpp
  Value.cpp)

add_library(${LIB_LLVM_BROWSE_LIB}
  ${SOURCES})

target_link_libraries(${LIB_LLVM_BROWSE_LIB}
  ${LLVM_LIB_SHARED})
