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

# qt_standard_project_setup() enables AUTOMOC and AUTOUIC, but not AUTORCC.
# Our built-in JSON profiles are embedded through the .qrc file below.
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

set(MYAPP_TEMPLATE_RESOURCES
  resources/myapp_template.qrc
)

set(MYAPP_TEMPLATE_CORE_SOURCES
  src/core/AppLogger.cpp
  src/core/AppLogger.h
  src/core/ConfigManager.cpp
  src/core/ConfigManager.h
  src/core/DiagnosticsManager.cpp
  src/core/DiagnosticsManager.h
)

qt_add_executable(myapp-template
  src/app/main.cpp
  src/ui/MainWindow.cpp
  src/ui/MainWindow.h
  src/settings/SettingsDialog.cpp
  src/settings/SettingsDialog.h
  src/workspace/TemplateDashboard.cpp
  src/workspace/TemplateDashboard.h
  src/core/ThemeManager.cpp
  src/core/ThemeManager.h
  ${MYAPP_TEMPLATE_CORE_SOURCES}
  ${MYAPP_TEMPLATE_RESOURCES}
)

target_include_directories(myapp-template
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(myapp-template
  PRIVATE
    Qt6::Core
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

include(CTest)

if(BUILD_TESTING)
  find_package(Qt6 REQUIRED COMPONENTS Test)

  qt_add_executable(config-manager-tests
    tests/unit/ConfigManagerTest.cpp
    ${MYAPP_TEMPLATE_CORE_SOURCES}
    ${MYAPP_TEMPLATE_RESOURCES}
  )

  target_include_directories(config-manager-tests
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )

  target_link_libraries(config-manager-tests
    PRIVATE
      Qt6::Core
      Qt6::Test
  )

  add_test(NAME ConfigManagerTests COMMAND config-manager-tests)

  qt_add_executable(logging-diagnostics-tests
    tests/unit/LoggingDiagnosticsTest.cpp
    ${MYAPP_TEMPLATE_CORE_SOURCES}
    ${MYAPP_TEMPLATE_RESOURCES}
  )

  target_include_directories(logging-diagnostics-tests
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )

  target_link_libraries(logging-diagnostics-tests
    PRIVATE
      Qt6::Core
      Qt6::Test
  )

  add_test(NAME LoggingDiagnosticsTests COMMAND logging-diagnostics-tests)
endif()
