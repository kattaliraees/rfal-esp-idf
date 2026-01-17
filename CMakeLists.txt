cmake_minimum_required(VERSION 3.16)

file(GLOB SRC_FILES "source/*.c" "source/st25r3911/*.c" "port/*.c")


idf_component_register(
    SRCS ${SRC_FILES}
    INCLUDE_DIRS "include" "source" "source/st25r3911" "port"
    REQUIRES driver freertos
)