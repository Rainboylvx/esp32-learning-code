# ESP-IDF 工程根目录：载入构建系统并声明工程名。
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(06-esp-timer-periodic-led)
