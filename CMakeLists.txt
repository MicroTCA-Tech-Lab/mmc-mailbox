cmake_minimum_required(VERSION 3.0)

project(mmc-mailbox VERSION 1.0.0)

include(GNUInstallDirs)

# mmcmb library

# COMPAT_ID is the device tree "compatible=" identifier for the mailbox device
# Invoke cmake with e.g. -DCOMPAT_ID="desy,mmcmailbox" to override the default
if(COMPAT_ID)
    message(STATUS "Set mailbox compat. ID to ${COMPAT_ID}")
    target_compile_definitions(mmcmb PRIVATE MB_DT_COMPAT_ID="${COMPAT_ID}")
endif()

add_library(mmcmb SHARED mmcmb.c)
set_target_properties(mmcmb PROPERTIES PUBLIC_HEADER "mmcmb/mmcmb.h;mmcmb/fpga_mailbox_layout.h")
set_target_properties(mmcmb PROPERTIES VERSION ${PROJECT_VERSION})

install(TARGETS mmcmb
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mmcmb
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# mmcinfo application

add_executable(mmcinfo mmcinfo.c)
target_link_libraries(mmcinfo mmcmb)
install(TARGETS mmcinfo DESTINATION ${CMAKE_INSTALL_BINDIR})

# mmcctrld application (daemon)

add_executable(mmcctrld mmcctrld.c)
target_link_libraries(mmcctrld mmcmb)
install(TARGETS mmcctrld DESTINATION ${CMAKE_INSTALL_SBINDIR})

# documentation

find_package(Python3 COMPONENTS Interpreter)

# update memory layout .md from .ods spreadsheet
add_custom_target(doc
    ${Python3_EXECUTABLE} "doc/ods2md/ods2md.py" "doc/mmc-fpga-data-interface.ods" > "doc/mmc-fpga-data-interface.md"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building documentation"
)