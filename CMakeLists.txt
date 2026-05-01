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
    elseif(APPLE)
        set(OBS_FRONTEND_LIB "OBS" CACHE STRING "OBS frontend library")
    else()
        set(OBS_FRONTEND_LIB "obs-frontend-api" CACHE STRING "OBS frontend library")
    endif()
endif()

# Find OBS libraries
if(DEFINED OBS_BUILD_DIR)
    message(STATUS "Using OBS build directory: ${OBS_BUILD_DIR}")
    set(LIBOBS_INCLUDE_DIRS
        ${OBS_BUILD_DIR}/../libobs
        ${OBS_BUILD_DIR}/../UI/obs-frontend-api
    )
    # Search for libobs library in multiple possible locations
    set(OBS_LIB_CANDIDATES
        "${OBS_BUILD_DIR}/libobs.framework/Versions/A/libobs"
        "${OBS_BUILD_DIR}/libobs.framework/libobs"
        "${OBS_BUILD_DIR}/libobs.framework"
        "${OBS_BUILD_DIR}/libobs/libobs.framework/Versions/A/libobs"
        "${OBS_BUILD_DIR}/libobs/libobs.framework/libobs"
        "${OBS_BUILD_DIR}/libobs/Release/libobs.framework/Versions/A/libobs"
        "${OBS_BUILD_DIR}/libobs/Debug/libobs.framework/Versions/A/libobs"
        "${OBS_BUILD_DIR}/libobs.dylib"
        "${OBS_BUILD_DIR}/liblibobs.dylib"
    )
    
    set(OBS_FOUND_LIB "")
    foreach(candidate ${OBS_LIB_CANDIDATES})
        if(EXISTS ${candidate})
            set(OBS_FOUND_LIB ${candidate})
            message(STATUS "Found libobs at: ${candidate}")
            break()
        endif()
    endforeach()
    
    if(NOT OBS_FOUND_LIB)
        # Try find_library which handles frameworks on macOS
        find_library(OBS_FOUND_LIB obs libobs
            PATHS 
                ${OBS_BUILD_DIR}
                ${OBS_BUILD_DIR}/libobs
                ${OBS_BUILD_DIR}/libobs/Release
                ${OBS_BUILD_DIR}/libobs/Debug
                ${OBS_BUILD_DIR}/libobs/RelWithDebInfo
            NO_DEFAULT_PATH
            NO_CACHE
        )
    endif()
    
    if(NOT OBS_FOUND_LIB)
        # List what exists for debugging
        file(GLOB OBS_BUILD_GLOB 
            "${OBS_BUILD_DIR}/libobs*" 
            "${OBS_BUILD_DIR}/*.framework"
            "${OBS_BUILD_DIR}/**/*.framework"
        )
        message(WARNING "Could not find libobs library. Searched: ${OBS_LIB_CANDIDATES}")
        message(STATUS "Contents of ${OBS_BUILD_DIR}: ${OBS_BUILD_GLOB}")
        # Fallback to framework directory for CMake to handle
        set(OBS_FOUND_LIB "${OBS_BUILD_DIR}/libobs.framework")
    endif()
    
    set(LIBOBS_LIBRARIES ${OBS_FOUND_LIB})
else()
    find_package(LibObs REQUIRED)
endif()

find_package(CURL REQUIRED)
if(NOT APPLE)
    find_package(cJSON REQUIRED)
endif()
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)
endif()

# Platform-specific settings
if(OS_WINDOWS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(PLATFORM_SOURCES)
elseif(APPLE)
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
    ${CURL_INCLUDE_DIRS}
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
)

# macOS: Add OpenSSL and cJSON include and library paths
if(APPLE)
    # OpenSSL for libwebsockets
    find_path(OPENSSL_INCLUDE_DIR openssl/ssl.h PATHS /opt/homebrew/opt/openssl/include /usr/local/opt/openssl/include)
    find_library(OPENSSL_SSL libssl PATHS /opt/homebrew/opt/openssl/lib /usr/local/opt/openssl/lib)
    find_library(OPENSSL_CRYPTO libcrypto PATHS /opt/homebrew/opt/openssl/lib /usr/local/opt/openssl/lib)
    if(OPENSSL_INCLUDE_DIR)
        target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
    endif()
    if(OPENSSL_SSL AND OPENSSL_CRYPTO)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_SSL} ${OPENSSL_CRYPTO})
    endif()

    # cJSON
    find_path(CJSON_INCLUDE_DIR cJSON.h PATHS
        /opt/homebrew/include
        /opt/homebrew/include/cjson
        /opt/homebrew/opt/cjson/include
        /usr/local/include
        /usr/local/include/cjson
        /usr/local/opt/cjson/include
    )
    find_library(CJSON_LIBRARY cjson PATHS
        /opt/homebrew/lib
        /opt/homebrew/opt/cjson/lib
        /usr/local/lib
        /usr/local/opt/cjson/lib
    )
    if(CJSON_INCLUDE_DIR AND CJSON_LIBRARY)
        target_include_directories(${PROJECT_NAME} PRIVATE ${CJSON_INCLUDE_DIR})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${CJSON_LIBRARY})
    else()
        # Fallback: add common Homebrew paths directly
        target_include_directories(${PROJECT_NAME} PRIVATE
            /opt/homebrew/include
            /opt/homebrew/include/cjson
            /usr/local/include
            /usr/local/include/cjson
        )
        target_link_directories(${PROJECT_NAME} PRIVATE
            /opt/homebrew/lib
            /usr/local/lib
        )
        target_link_libraries(${PROJECT_NAME} PRIVATE cjson)
    endif()
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${LIBOBS_LIBRARIES}
    CURL::libcurl
    pthread
    ${LIBWEBSOCKETS_LIBRARIES}
)

# Add cjson only if not on macOS (handled by pkg-config above)
if(NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE cjson)
endif()

if(OS_WINDOWS)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ws2_32
        crypt32
    )
elseif(APPLE)
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
