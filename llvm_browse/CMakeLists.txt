set(SOURCES
	llvm_browse.cpp)

set(TARGET llvm_browse)

include_directories(
	${Python_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR})

Python_add_library(${TARGET} MODULE 
	${SOURCES})

target_link_libraries(${TARGET}
	PUBLIC
	${LIB_LLVM_BROWSE_LIB})

set_target_properties(${TARGET} 
	PROPERTIES
	SUFFIX "${Python_SOABI}")
