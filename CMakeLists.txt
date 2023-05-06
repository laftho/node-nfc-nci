cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

include(FindPkgConfig)

# Name of the project (will be the name of the plugin)
project(node_nfc_nci)

pkg_check_modules(LIBNFC_NCI REQUIRED libnfc-nci)

add_definitions(-DNAPI_VERSION=4)

file(GLOB SOURCE_FILES "./src/*.cpp")

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})

if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
  # Generate node.lib
  execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif()

target_link_libraries(${PROJECT_NAME} ${LIBNFC_NCI_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${LIBNFC_NCI_INCLUDE_DIRS})

# target_link_libraries(${PROJECT_NAME} -L/usr/local/lib/libnfc_nci_linux.so)
