cmake_minimum_required(VERSION 3.14.4)
project(MacGyver)

set(CMAKE_MFC_FLAG 0)

disable_linux_side_compiler_warnings()

file( 
    GLOB SOURCE_FILES 
    macgyver/* 
    )

list(
    REMOVE_ITEM SOURCE_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/CharsetConverter.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/CharsetConverter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/TemplateFormatter.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/TemplateFormatter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/TemplateFormatterMT.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/TemplateFormatterMT.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/TemplateDirectory.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/TemplateDirectory.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/TemplateFactory.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/TemplateFactory.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/PostgreSQLConnection.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/PostgreSQLConnection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/ValueFormatter.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/ValueFormatter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/AsyncTaskGroup.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/macgyver/AsyncTaskGroup.cpp"
    )

add_library( 
    ${PROJECT_NAME} STATIC
    ${SOURCE_FILES} 
    )

target_link_libraries( 
    ${PROJECT_NAME}
    )

target_include_directories( 
    ${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/macgyver
    PUBLIC ${PROJECT_SOURCE_DIR}
    PUBLIC ${BOOST_INCLUDE_DIR}
    PUBLIC ${FMT_INCLUDE_DIR}
    )

set(
    ${PROJECT_NAME}_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/macgyver
    ${PROJECT_SOURCE_DIR}
    ${BOOST_INCLUDE_DIR}
    ${FMT_INCLUDE_DIR}
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE
    )
