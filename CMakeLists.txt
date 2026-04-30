cmake_minimum_required(VERSION 3.10)
project(obs-streamlabels)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(CMAKE_COMPILE_WARNING_AS_ERROR)
    add_compile_options(/WX)
else()
    add_compile_options(-Wno-error)
endif()

# Find OBS
if(NOT DEFINED OBS_FRONTEND_LIB)
    if(OS_WINDOWS)
        set(OBS_FRONTEND_LIB "obs-frontend-api" CACHE STRING "OBS frontend library")
    elseif(OS_MACOS)
        set(OBS_FRONTEND_LIB "OBS" CACHE STRING "OBS frontend library")
    else()
        set(OBS_FRONTEND_LIB "obs-frontend-api" CACHE STRING "OBS frontend library")
    endif()
endif()

# Find required packages
find_package(LibObs REQUIRED)
find_package(libcurl REQUIRED)
find_package(cJSON REQUIRED)
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)
endif()

# Platform-specific settings
if(OS_WINDOWS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(PLATFORM_SOURCES)
elseif(OS_MACOS)
    set(PLATFORM_SOURCES)
elseif(OS_LINUX)
    set(PLATFORM_SOURCES)
endif()

# Source files
set(SOURCES
    src/plugin-main.cpp
    src/api-client.cpp
    src/websocket-client.cpp
    src/streamlabel-source.cpp
    src/settings.cpp
    src/train-logic.cpp
)

# Header files
set(HEADERS
    src/api-client.h
    src/websocket-client.h
    src/streamlabel-source.h
    src/settings.h
    src/train-logic.h
)

# Create shared library
add_library(${PROJECT_NAME} SHARED
    ${SOURCES}
    ${HEADERS}
    ${PLATFORM_SOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${LIBOBS_INCLUDE_DIRS}
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${LIBOBS_LIBRARIES}
    ${CURL_LIBRARIES}
    cjson
    pthread
    ${LIBWEBSOCKETS_LIBRARIES}
)

if(OS_WINDOWS)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ws2_32
        crypt32
    )
elseif(OS_MACOS)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework Cocoa"
    )
endif()

# Install plugin
if(OS_POSIX)
    install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/obs-plugins/${PROJECT_NAME}"
    )
    install(FILES ${HEADERS}
        DESTINATION "${CMAKE_INSTALL_PREFIX}/obs-plugins/${PROJECT_NAME}"
    )
elseif(OS_WINDOWS)
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "obs-plugins/${PROJECT_NAME}"
        LIBRARY DESTINATION "obs-plugins/${PROJECT_NAME}"
    )
    install(FILES ${HEADERS}
        DESTINATION "obs-plugins/${PROJECT_NAME}"
    )
endif()

# Install locale files
install(DIRECTORY data/locale/
    DESTINATION "data/locale"
)
