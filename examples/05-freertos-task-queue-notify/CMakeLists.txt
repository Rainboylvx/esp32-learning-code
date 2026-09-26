# ESP-IDF 工程根目录的固定写法：声明最低 CMake 版本，
# 载入 ESP-IDF 构建系统，再声明工程名。
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(05-freertos-task-queue-notify)
