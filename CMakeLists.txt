cmake_minimum_required(VERSION 3.0)

project(mmc-mailbox VERSION 1.0.0)

include(GNUInstallDirs)

add_library(mmcmb SHARED mmcmb.c)
set_target_properties(mmcmb PROPERTIES PUBLIC_HEADER "mmcmb/mmcmb.h;mmcmb/fpga_mailbox_layout.h")
set_target_properties(mmcmb PROPERTIES VERSION ${PROJECT_VERSION})

# Invoke cmake with e.g. -DADAPTER_DT_NAME="iic_axi_iic_mmc" -DI2C_ADDR="002a" to override defaults
if(ADAPTER_DT_NAME)
    message(STATUS "Set adapter devicetree name to ${ADAPTER_DT_NAME}")
    target_compile_definitions(mmcmb PRIVATE MB_ADAPTER_DT_NAME="${ADAPTER_DT_NAME}")
endif()
if(I2C_ADDR)
    message(STATUS "Set mailbox I2C address to ${I2C_ADDR}")
    target_compile_definitions(mmcmb PRIVATE MB_I2C_ADDR="${I2C_ADDR}")
endif()

install(TARGETS mmcmb
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mmcmb
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_executable(mmcinfo mmcinfo.c)
target_link_libraries(mmcinfo mmcmb)
install(TARGETS mmcinfo DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(mmcctrld mmcctrld.c)
target_link_libraries(mmcctrld mmcmb)
install(TARGETS mmcctrld DESTINATION ${CMAKE_INSTALL_SBINDIR})
