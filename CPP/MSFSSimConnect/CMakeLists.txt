cmake_minimum_required(VERSION 3.21)
project(MSFSSimConnectQt LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Network Widgets)

add_executable(MSFS-SimConnect
    qt/main.cpp
    qt/main_window.cpp
    qt/main_window.h
    qt/qr_code_widget.cpp
    qt/qr_code_widget.h
    qt/port_mapper.cpp
    qt/port_mapper.h
    qt/udp_server.cpp
    qt/udp_server.h
    qt/simconnect_source.cpp
    qt/simconnect_source.h
    ThirdParty/qrcodegen/qrcodegen.cpp
)

target_link_libraries(MSFS-SimConnect PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::Widgets)
install(TARGETS MSFS-SimConnect RUNTIME DESTINATION .)

# Provide this cache variable with the directory containing SimConnect.h and
# SimConnect.lib when building the Windows data-source integration.
set(SIMCONNECT_SDK_DIR "" CACHE PATH "MSFS SimConnect SDK directory")
if (WIN32 AND SIMCONNECT_SDK_DIR)
    target_compile_definitions(MSFS-SimConnect PRIVATE HAVE_SIMCONNECT=1)
    target_include_directories(MSFS-SimConnect PRIVATE "${SIMCONNECT_SDK_DIR}/include")
    target_link_directories(MSFS-SimConnect PRIVATE "${SIMCONNECT_SDK_DIR}/lib")
    target_link_libraries(MSFS-SimConnect PRIVATE SimConnect)

    # This small console companion is for a native Linux Qt application that
    # needs to access SimConnect through Wine.  It sends data only to localhost.
    add_executable(MSFS-SimConnect-WineBridge qt/wine_bridge.cpp)
    target_compile_features(MSFS-SimConnect-WineBridge PRIVATE cxx_std_20)
    target_include_directories(MSFS-SimConnect-WineBridge PRIVATE "${SIMCONNECT_SDK_DIR}/include")
    target_link_directories(MSFS-SimConnect-WineBridge PRIVATE "${SIMCONNECT_SDK_DIR}/lib")
    target_link_libraries(MSFS-SimConnect-WineBridge PRIVATE SimConnect Ws2_32)
    install(TARGETS MSFS-SimConnect-WineBridge RUNTIME DESTINATION .)
endif()
