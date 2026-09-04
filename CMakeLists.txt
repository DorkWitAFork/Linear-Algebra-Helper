cmake_minimum_required(VERSION 3.24)

project(LAHelper VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(SymEngine CONFIG REQUIRED)

add_library(la_core
    src/symbolic.cpp
    src/matrix.cpp
    src/algorithms.cpp
    src/expression.cpp
)
target_include_directories(la_core PUBLIC src ${SYMENGINE_INCLUDE_DIRS})
target_link_libraries(la_core PUBLIC ${SYMENGINE_LIBRARIES})

add_executable(LAHelper WIN32
    src/main.cpp
    src/mainwindow.cpp
)
target_link_libraries(LAHelper PRIVATE la_core Qt6::Widgets)

include(CTest)
if(BUILD_TESTING)
    add_executable(la_core_tests tests/core_tests.cpp)
    target_link_libraries(la_core_tests PRIVATE la_core)
    add_test(NAME la_core_tests COMMAND la_core_tests)
endif()

install(TARGETS LAHelper RUNTIME DESTINATION .)
