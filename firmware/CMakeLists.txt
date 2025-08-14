#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aws_iot)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE led/led.c)
target_sources(app PRIVATE led/color.c)

zephyr_include_directories(src)
zephyr_include_directories(led)
