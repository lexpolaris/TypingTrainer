cmake_minimum_required(VERSION 3.16)
project(TypingTrainer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Charts)

# 可选：图表、数据库（后续阶段启用）
find_package(Qt6 COMPONENTS Charts Sql QUIET)
if(Qt6Charts_FOUND)
    message(STATUS "QtCharts found, statistics chart enabled")
endif()
if(Qt6Sql_FOUND)
    message(STATUS "QtSql found, history DB enabled")
endif()

set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/app/ConfigManager.cpp
    src/core/TextDocument.cpp
    src/core/TypingSession.cpp
    src/core/CodeTable.cpp
    src/theme/ThemeManager.cpp
    src/ui/MainWindow.cpp
    src/ui/TextLibraryDialog.cpp
    src/ui/TypingView.cpp
    src/ui/PacmanView.cpp
    src/ui/TwoLineView.cpp
    src/ui/CodeHintPanel.cpp
    src/utils/AppPaths.cpp
    src/utils/TextLoader.cpp
    src/core/SpeedPointFinder.cpp
    src/ui/SpeedPointDialog.cpp
    src/ui/SpeedChartDialog.cpp
    src/ui/WrappedTextView.cpp
    src/ui/SettingsDialog.cpp
)

set(HEADERS
    src/app/Application.h
    src/app/ConfigManager.h
    src/core/TextDocument.h
    src/core/TypingSession.h
    src/core/CodeTable.h
    src/theme/ThemeManager.h
    src/ui/MainWindow.h
    src/ui/TextLibraryDialog.h
    src/ui/TypingView.h
    src/ui/PacmanView.h
    src/ui/TwoLineView.h
    src/ui/CodeHintPanel.h
    src/utils/AppPaths.h
    src/utils/TextLoader.h
    src/core/SpeedPointFinder.h
    src/ui/SpeedPointDialog.h
    src/ui/SpeedChartDialog.h
    src/ui/WrappedTextView.h
    src/ui/SettingsDialog.h
)

add_executable(TypingTrainer ${SOURCES} ${HEADERS} resources/resources.qrc)

target_include_directories(TypingTrainer PRIVATE src)

target_link_libraries(TypingTrainer PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Charts
)

if(Qt6Charts_FOUND)
    target_link_libraries(TypingTrainer PRIVATE Qt6::Charts)
    target_compile_definitions(TypingTrainer PRIVATE HAVE_QTCHARTS)
endif()

if(Qt6Sql_FOUND)
    target_link_libraries(TypingTrainer PRIVATE Qt6::Sql)
    target_compile_definitions(TypingTrainer PRIVATE HAVE_QTSQL)
endif()

# 平台特定
if(WIN32)
    set_target_properties(TypingTrainer PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
    set_target_properties(TypingTrainer PROPERTIES MACOSX_BUNDLE TRUE)
endif()

install(TARGETS TypingTrainer
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .)
