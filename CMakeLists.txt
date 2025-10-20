cmake_minimum_required(VERSION 3.15)

project(mmc-mailbox VERSION 1.1.1)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# mmcmb library

add_library(mmcmb SHARED mmcmb.c)
set_target_properties(mmcmb PROPERTIES PUBLIC_HEADER "mmcmb/mmcmb.h;mmcmb/mmcmb.hpp;mmcmb/fpga_mailbox_layout.h")
set_target_properties(mmcmb PROPERTIES VERSION ${PROJECT_VERSION})
target_compile_options(mmcmb PRIVATE -Wall -Wextra -O2)

# COMPAT_ID is the device tree "compatible=" identifier for the mailbox device
# Invoke cmake with e.g. -DCOMPAT_ID="desy,mmcmailbox" to override the default
if(COMPAT_ID)
    message(STATUS "Set mailbox compat. ID to ${COMPAT_ID}")
    target_compile_definitions(mmcmb PRIVATE MB_DT_COMPAT_ID="${COMPAT_ID}")
endif()

configure_package_config_file(mmcmb-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/mmcmb-config.cmake
    INSTALL_DESTINATION ${LIB_INSTALL_DIR}/cmake/mmcmb
    PATH_VARS CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/mmcmb-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(TARGETS mmcmb
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mmcmb
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/mmcmb-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/mmcmb-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mmcmb"
)

# mmcinfo application

add_executable(mmcinfo mmcinfo.c)
target_link_libraries(mmcinfo mmcmb)
target_compile_options(mmcinfo PRIVATE -Wall -Wextra -O2)
install(TARGETS mmcinfo DESTINATION ${CMAKE_INSTALL_BINDIR})

# mmcctrld application (daemon)

add_executable(mmcctrld mmcctrld.c)
target_link_libraries(mmcctrld mmcmb)
target_compile_options(mmcctrld PRIVATE -Wall -Wextra -O2)
install(TARGETS mmcctrld DESTINATION ${CMAKE_INSTALL_SBINDIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD IMPORTED_TARGET libsystemd)
if(SYSTEMD_FOUND)
    if("${SYSTEMD_SERVICES_INSTALL_DIR}" STREQUAL "")
        execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE}
                        --variable=systemdsystemunitdir systemd
                        OUTPUT_VARIABLE SYSTEMD_SERVICES_INSTALL_DIR)
        string(REGEX REPLACE "[ \t\n]+" "" SYSTEMD_SERVICES_INSTALL_DIR "${SYSTEMD_SERVICES_INSTALL_DIR}")
    endif()
    target_compile_definitions(mmcctrld PRIVATE ENABLE_SYSTEMD)
    target_link_libraries(mmcctrld PkgConfig::SYSTEMD)
    message(STATUS "systemd support enabled, will install the service at '${SYSTEMD_SERVICES_INSTALL_DIR}'")
    install(FILES mmcctrld.service DESTINATION "${SYSTEMD_SERVICES_INSTALL_DIR}")
else()
    message(WARNING "systemd not found, disabling systemd support")
endif()

# documentation

find_package(Python3 COMPONENTS Interpreter)

# update .md & .svg from .ods & .drawio
add_custom_target(doc
    COMMAND
    ${Python3_EXECUTABLE} "doc/ods2md/ods2md.py" "doc/mmc-fpga-data-interface.ods" > "doc/mmc-fpga-data-interface.md"
    COMMAND
    drawio -x "doc/mmc-mailbox.drawio" -o "doc/mmc-mailbox.svg"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building documentation"
)
