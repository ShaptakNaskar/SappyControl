cmake_minimum_required(VERSION 3.21)

project(SappyControls VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)
include(CTest)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

add_executable(sappy-controls
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/commandbuilder.cpp
    src/commandbuilder.h
    resources/resources.qrc
)

target_link_libraries(sappy-controls PRIVATE Qt6::Widgets)
target_compile_options(sappy-controls PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

install(TARGETS sappy-controls RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS scripts/wifi-kill
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    RENAME sappy-wifi-kill
)
install(PROGRAMS scripts/hetzner-nettest.sh
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    RENAME sappy-hetzner-nettest
)
install(FILES packaging/io.github.sappy.SappyControls.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(FILES packaging/io.github.sappy.SappyControls.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)
install(FILES packaging/io.github.sappy.SappyControls.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    RENAME sappy-controls.svg
)
foreach(ICON_SIZE 48 64 128 256)
    install(FILES packaging/icons/${ICON_SIZE}x${ICON_SIZE}/apps/sappy-controls.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${ICON_SIZE}x${ICON_SIZE}/apps
    )
endforeach()

if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

    add_executable(commandbuilder-tests
        tests/commandbuilder_tests.cpp
        src/commandbuilder.cpp
        src/commandbuilder.h
    )
    target_include_directories(commandbuilder-tests PRIVATE src)
    target_link_libraries(commandbuilder-tests PRIVATE Qt6::Test)
    add_test(NAME commandbuilder-tests COMMAND commandbuilder-tests)
endif()
