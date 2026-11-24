cmake_minimum_required(VERSION 3.25)
project(ShirazTipsyResearchInstitute VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.12 REQUIRED COMPONENTS Widgets Sql PrintSupport)

enable_testing()

set(YADAVARIFORK_SOURCES
    src/main.cpp
    brand.qrc
    src/project_store.cpp
    src/project_store.h
    src/document_export.cpp
    src/document_export.h
    src/research_engine_runner.cpp
    src/research_engine_runner.h
)

if(WIN32)
    enable_language(RC)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/shiraztipsy.ico" APP_ICON_ICO_PATH)
    configure_file(app_icon.rc.in app_icon.rc @ONLY)
    list(APPEND YADAVARIFORK_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc")
endif()

qt_add_executable(yadavarifork ${YADAVARIFORK_SOURCES})
target_link_libraries(yadavarifork PRIVATE Qt6::Widgets Qt6::Sql Qt6::PrintSupport Qt6::Core)

# A console-subsystem test binary keeps diagnostics visible to CTest on Windows.
qt_add_executable(yadavarifork_smoke EXCLUDE_FROM_ALL ${YADAVARIFORK_SOURCES})
target_link_libraries(yadavarifork_smoke PRIVATE Qt6::Widgets Qt6::Sql Qt6::PrintSupport Qt6::Svg Qt6::Core)
if(WIN32)
    set_target_properties(yadavarifork_smoke PROPERTIES WIN32_EXECUTABLE FALSE)
endif()

if(WIN32)
    set_target_properties(yadavarifork PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

add_test(NAME app_smoke COMMAND yadavarifork_smoke --smoke-test)
set_tests_properties(app_smoke PROPERTIES TIMEOUT 15
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;YADAVARIFORK_SMOKE_OUTPUT_DIR=${CMAKE_BINARY_DIR}/smoke-output")

# Exercise the QProcess research runner independently of the GUI. The test
# executable also acts as its own child process to keep the harness portable.
qt_add_executable(research_engine_runner_tests EXCLUDE_FROM_ALL
    tests/research_engine_runner_test.cpp
    src/research_engine_runner.cpp
    src/research_engine_runner.h
)
target_link_libraries(research_engine_runner_tests PRIVATE Qt6::Core)
target_include_directories(research_engine_runner_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(WIN32)
    set_target_properties(research_engine_runner_tests PROPERTIES WIN32_EXECUTABLE FALSE)
endif()
add_test(NAME research_engine_runner_contract COMMAND research_engine_runner_tests)
set_tests_properties(research_engine_runner_contract PROPERTIES TIMEOUT 15)
