
add_executable(headers headers.c)

include_directories(${craneweb_BINARY_DIR})
include_directories(${craneweb_SOURCE_DIR})
include_directories(${craneweb_SOURCE_DIR}/src)

link_directories(${craneweb_BINARY_DIR}/src)

target_link_libraries(headers craneweb)
target_link_libraries(headers pthread) # XXX

