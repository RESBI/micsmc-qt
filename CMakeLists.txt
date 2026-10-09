cmake_minimum_required(VERSION 3.16)
project(micsmc-qt LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(micsmc-qt
    src/main.cpp
    src/Snapshot.h
    src/I18n.h
    src/DeviceWorker.h src/DeviceWorker.cpp
    src/PlotWidget.h    src/PlotWidget.cpp
    src/CoreGridWidget.h src/CoreGridWidget.cpp
    src/CardView.h      src/CardView.cpp
    src/SettingsDialog.h src/SettingsDialog.cpp
    src/MainWindow.h     src/MainWindow.cpp
    res/micsmc-qt.qrc
)

target_link_libraries(micsmc-qt PRIVATE
    Qt6::Widgets
    micmgmt
    scif
)

# 头文件路径：miclib.h 由 mpss-micmgmt 安装到 /usr/include
target_include_directories(micsmc-qt PRIVATE src)

# ---------------------------------------------------------------------------
# 安装：make install（默认前缀 /usr/local，可用 -DCMAKE_INSTALL_PREFIX= 覆盖）
#   <prefix>/bin/micsmc-qt                     可执行文件
#   <prefix>/share/applications/micsmc-qt.desktop  桌面入口
# 注意：QSS 通过 qrc 编进可执行文件，无额外数据文件需要安装。
# ---------------------------------------------------------------------------
install(TARGETS micsmc-qt
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES res/micsmc-qt.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# ---------------------------------------------------------------------------
# 卸载：make uninstall
# 以 install_manifest.txt 为准（install 时写下的真实路径），
# 并按白名单校验每个条目，详见 cmake_uninstall.cmake.in 顶部说明。
# ---------------------------------------------------------------------------
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
    @ONLY)

if(NOT TARGET uninstall)
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
        COMMENT "卸载 micsmc-qt（依据 install_manifest.txt）")
endif()
