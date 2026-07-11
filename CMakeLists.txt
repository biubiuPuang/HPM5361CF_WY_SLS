# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)
set(SES_TOOLCHAIN_VARIANT "Andes")

# 功能模块配置
set(CONFIG_HPM_MATH 1)
#set(CONFIG_HPM_MATH_DSP 1)
#set(HPM_MATH_DSP_SES_LIB "libdspf")
# set(CONFIG_MOTORCTRL 1)
set(CONFIG_HPM_ADC 1)
set(CONFIG_HPM_ADC16 1)
set(CONFIG_HPM_UART 1)
set(CONFIG_HPM_PWM 1)
set(CONFIG_HPM_I2C 1)
set(CONFIG_HPM_DMA 1)
#set(CONFIG_CHERRYUSB 1)
#set(CONFIG_USB_DEVICE 1)
#set(CONFIG_USB_DEVICE_CDC 1)
# set(CONFIG_EEPROM_EMULATION 1)
# set(CONFIG_DMA_MGR 1)
set(CONFIG_SEGGER_RTT 1)
set(CONFIG_NDEBUG_CONSOLE 1)

# 查找SDK包
find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
project(hpm5361)

# 编译选项
sdk_ses_compile_options(-mabi=ilp32f)
sdk_ses_compile_options(-march=rv32imafc)
sdk_ses_compile_options(-O0)
sdk_ses_compile_options(-DBOARD_SHOW_CLOCK)
sdk_ses_opt_debug_connection(J-Link)

# 宏定义
sdk_compile_definitions(-DCONFIG_NDEBUG_CONSOLE=1)
sdk_compile_definitions(-DBOARD_PWM_PERIOD=1000)
sdk_compile_definitions(-DCONFIG_HPM_PWM=1)
sdk_compile_definitions(-DCONFIG_HPM_I2C=1)
sdk_ses_opt_lib_io_type(RTT)

# 提供正确的包含路径
sdk_inc(.)
sdk_inc(boards)
sdk_inc(boards/MotorMonitor)
sdk_inc(src)
sdk_inc(src/app)
sdk_inc(src/bsp)

# 用户自定义添加_双踏板检测脚本
sdk_inc(Python-C_Second-Time)
sdk_inc(Python-C_Second-Time/build)
sdk_inc(Python-C_Second-Time/examples)
sdk_inc(Python-C_Second-Time/include)
sdk_inc(Python-C_Second-Time/src)

# 收集源文件
file(GLOB_RECURSE SRC_FILES 
    src/app/*.c
    src/bsp/*.c
)
# 排除renamed目录
list(FILTER SRC_FILES EXCLUDE REGEX \"/renamed/\")

sdk_app_src(${SRC_FILES})


generate_ses_project()



