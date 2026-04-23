cmake_minimum_required(VERSION 3.16)
project(LLM_chartcreator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Source files
set(SOURCES
    src/main.cpp
    src/api/OllamaClient.cpp
    src/core/CodeExecutor.cpp
    src/core/PromptBuilder.cpp
    src/gui/MainWindow.cpp
    src/gui/OutputWidget.cpp
    src/gui/SettingsDialog.cpp
    src/utils/Logger.cpp
    src/utils/FileUtils.cpp
)

set(HEADERS
    src/api/OllamaClient.h
    src/core/CodeExecutor.h
    src/core/PromptBuilder.h
    src/gui/MainWindow.h
    src/gui/OutputWidget.h
    src/gui/SettingsDialog.h
    src/utils/Logger.h
    src/utils/FileUtils.h
)

# Main executable
# WIN32 = aplikacja okienkowa na Windows (brak okna konsoli)
# Na Linux/macOS flaga jest ignorowana
add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# Tests executable
set(TEST_SOURCES
    tests/test_main.cpp
    tests/test_OllamaClient.cpp
    tests/test_PromptBuilder.cpp
    tests/test_CodeExecutor.cpp
    tests/test_FileUtils.cpp
    # Shared sources needed by tests
    src/api/OllamaClient.cpp
    src/core/CodeExecutor.cpp
    src/core/PromptBuilder.cpp
    src/utils/Logger.cpp
    src/utils/FileUtils.cpp
)

add_executable(${PROJECT_NAME}_tests
    ${TEST_SOURCES}
)

target_include_directories(${PROJECT_NAME}_tests PRIVATE src)

target_link_libraries(${PROJECT_NAME}_tests PRIVATE
    GTest::gtest_main
    GTest::gmock
    Qt6::Core
    Qt6::Network
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_tests
    DISCOVERY_MODE PRE_TEST
)

# Doxygen
find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/docs/doxygen)
    set(DOXYGEN_PROJECT_NAME "LLM chart creator")
    set(DOXYGEN_PROJECT_BRIEF "Generator kodu Python z wykorzystaniem lokalnego LLM")
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    doxygen_add_docs(doxygen ${CMAKE_SOURCE_DIR}/src COMMENT "Generating Doxygen docs")
endif()