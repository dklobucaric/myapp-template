cmake_minimum_required(VERSION 3.24)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
  MYAPP_TEMPLATE_VERSION
  LIMIT_COUNT 1
)
string(STRIP "${MYAPP_TEMPLATE_VERSION}" MYAPP_TEMPLATE_VERSION)

project(MyAppTemplate
  VERSION "${MYAPP_TEMPLATE_VERSION}"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(myapp-template
  src/app/main.cpp
  src/ui/MainWindow.cpp
  src/ui/MainWindow.h
  src/workspace/TemplateDashboard.cpp
  src/workspace/TemplateDashboard.h
)

target_include_directories(myapp-template
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(myapp-template
  PRIVATE
    Qt6::Widgets
)

target_compile_definitions(myapp-template
  PRIVATE
    MYAPP_TEMPLATE_VERSION="${PROJECT_VERSION}"
)

set_target_properties(myapp-template PROPERTIES
  WIN32_EXECUTABLE TRUE
  MACOSX_BUNDLE TRUE
)

