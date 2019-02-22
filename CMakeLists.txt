cmake_minimum_required(VERSION 3.10)
project(node_nfc_nci)

set(CMAKE_CXX_STANDARD 14)

include_directories(./node_modules/node-addon-api/src)
include_directories(${MY_SOURCE_DIR}/node_modules/node-addon-api)

add_library(node_nfc_nci src/library.cpp)

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Essential include files to build a node addon,
# You should add this line in every CMake.js based project
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})

# Essential library files to link to a node addon
# You should add this line in every CMake.js based project
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

# Include N-API wrappers
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/node_modules/node-addon-api")
