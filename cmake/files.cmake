# Add sources to executable/library
target_sources(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Application/Startup/startup_stm32wl33ccvx.s"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32wl3x_hal_msp.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32wl3x_it.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Application/User/syscalls.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Application/User/sysmem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/STM32WL3x-Nucleo/stm32wl3x_nucleo.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/system_stm32wl3x.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_hal.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_hal_cortex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_hal_dma.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_hal_flash.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_hal_flash_ex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_hal_gpio.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_hal_mrsubg.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_hal_pwr.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_hal_pwr_ex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_hal_rcc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_hal_rcc_ex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_hal_tim.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_hal_tim_ex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32WL3x_HAL_Driver/Src/stm32wl3x_ll_utils.c"
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/STM32WL33CCVX_FLASH.ld" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/STM32WL33CCVX_FLASH.ld")
