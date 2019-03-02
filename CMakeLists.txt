cmake_minimum_required(VERSION 2.8)

# Name of the project (will be the name of the plugin)
project(node_nfc_nci)

# Build a shared library named after the project from the files in `src/`
file(GLOB SOURCE_FILES "src/*.cpp" "src/*.h")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

target_include_directories(${PROJECT_NAME} PRIVATE "/usr/include/node")

# Include N-API wrappers
if (EXISTS "${CMAKE_SOURCE_DIR}/node_modules/")
  target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/node_modules/node-addon-api")
  #target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/node_modules/node-addon-api/src")
else()
  target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../node-addon-api")
  #target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../node-addon-api/src")
endif()

# Essential include files to build a node addon,
# You should add this line in every CMake.js based project
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})

# Essential library files to link to a node addon
# You should add this line in every CMake.js based project
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

target_link_libraries(${PROJECT_NAME} nfc_nci_linux)

# target_link_libraries(${PROJECT_NAME} -L/usr/local/lib/libnfc_nci_linux.so)
