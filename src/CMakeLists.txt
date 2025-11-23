# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

# FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# idf_component_register(SRCS ${app_sources})

# Root CMakeLists.txt for ESP32 project

cmake_minimum_required(VERSION 3.16)

# Set project name
project(ESPESP32_Network_Manager C)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Add main project
project(ESP32_Network_Manager)

# ==================== Source files ====================
# Gồm tất cả các file trong network và src
set(SRC_FILES
    src/main.c
    network/network_wifi.c
    network/network_4g.c
    network/network_manager.c
    network/aws_mqtt_client.c
)

# ==================== Include directories ====================
include_directories(
    network
    include
    src
)

# ==================== Component registration ====================
idf_component_register(
    SRCS ${SRC_FILES}
    INCLUDE_DIRS "network" "include" "src"
)
