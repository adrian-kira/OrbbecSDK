cmake_minimum_required(VERSION 3.28.3)
project(OrbbecFirmwareUpdater
    VERSION 1.0
    DESCRIPTION "Orbbec Firmware Updater"
    LANGUAGES CXX)

find_package(Boost REQUIRED COMPONENTS system program_options)

add_executable(OrbbecFirmwareUpdater src/orbbec_fw_updater.cpp src/ob_updater.cpp)
target_include_directories(OrbbecFirmwareUpdater PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/src)

target_link_libraries(OrbbecFirmwareUpdater PRIVATE 
    OrbbecSDK::OrbbecSDK
    Boost::system 
    Boost::program_options
    pthread
    dl
    rt
)

install(TARGETS OrbbecFirmwareUpdater DESTINATION bin)

# Install firmware binaries
install(DIRECTORY bin/ DESTINATION share/orbbec-fw-updater/firmware
        FILES_MATCHING PATTERN "*.bin")

set_target_properties(OrbbecFirmwareUpdater PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS YES)