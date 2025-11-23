file(GLOB_RECURSE NETWORK_SOURCES "*.c")

idf_component_register(
    SRCS ${NETWORK_SOURCES}
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi
)
