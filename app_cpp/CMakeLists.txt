cmake_minimum_required(VERSION 3.25)
project(YadavariforkPortable VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets)

enable_testing()

qt_add_executable(yadavarifork
    src/main.cpp
)
target_link_libraries(yadavarifork PRIVATE Qt6::Widgets)

if(WIN32)
    set_target_properties(yadavarifork PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

add_test(NAME app_smoke COMMAND yadavarifork --smoke-test)
set_tests_properties(app_smoke PROPERTIES TIMEOUT 15)
