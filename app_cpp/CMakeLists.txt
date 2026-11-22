cmake_minimum_required(VERSION 3.25)
project(YadavariforkPortable VERSION 0.1.0 LANGUAGES CXX)

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
    src/project_store.cpp
    src/project_store.h
    src/document_export.cpp
    src/document_export.h
)

qt_add_executable(yadavarifork ${YADAVARIFORK_SOURCES})
target_link_libraries(yadavarifork PRIVATE Qt6::Widgets Qt6::Sql Qt6::PrintSupport)

# A console-subsystem test binary keeps diagnostics visible to CTest on Windows.
qt_add_executable(yadavarifork_smoke EXCLUDE_FROM_ALL ${YADAVARIFORK_SOURCES})
target_link_libraries(yadavarifork_smoke PRIVATE Qt6::Widgets Qt6::Sql Qt6::PrintSupport)

if(WIN32)
    set_target_properties(yadavarifork PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

add_test(NAME app_smoke COMMAND yadavarifork_smoke --smoke-test)
set_tests_properties(app_smoke PROPERTIES TIMEOUT 15 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
