
set(MSINT_DIR ${craneweb_SOURCE_DIR}/deps/msinttypes)

set(LIBUSF_DIR ${craneweb_SOURCE_DIR}/deps/libuseful)
set(LIBUSF_SOURCES ${LIBUSF_DIR}/src/list.c ${LIBUSF_DIR}/src/stringbuilder.c ${LIBUSF_DIR}/src/platform.c)

set(REGEX_DIR ${craneweb_SOURCE_DIR}/deps/regex-3.8a)
set(REGEX_SOURCES ${REGEX_DIR}/regcomp.c ${REGEX_DIR}/regexec.c ${REGEX_DIR}/regerror.c ${REGEX_DIR}/regfree.c)

set(CRANEWEB_DBG_SOURCES ${craneweb_SOURCE_DIR}/src/craneweb.c)

set(BUILD_HEADER ${craneweb_SOURCE_DIR}/tests/build_craneweb_private_h.py)

option(ENABLE_TESTS "compile testsuite")

if(ENABLE_TESTS)
    find_package(LIBCHECK)
    if(LIBCHECK_NOTFOUND)
        message(FATAL_ERROR, "libcheck not found (get it at http://check.sf.net) - can't compile testsuite")
    endif(LIBCHECK_NOTFOUND)

    include_directories(${LIBCHECK_INCLUDE_DIR})

    # for the custom debug config.h. MUST BE the first.
    include_directories(${craneweb_SOURCE_DIR}/tests)
    # for the custom header.
    include_directories(${craneweb_BINARY_DIR}/tests)
    include_directories(${craneweb_SOURCE_DIR})
    include_directories(${craneweb_SOURCE_DIR}/src)
    if(MSVC60 OR MSVC70 OR MSVC71 OR MSVC80)
        include_directories(${MSINT_DIR})
    endif(MSVC60 OR MSVC70 OR MSVC71 OR MSVC80)
    include_directories(${LIBUSF_DIR}/src/include)
    include_directories(${REGEX_DIR})

    # we need a custom debug build
    # base
    add_definitions(-DHAVE_CONFIG_H)
    add_definitions(-DCRW_PRIVATE=extern)
    add_definitions(-DCRW_DEBUG=1)
    # regex
    add_definitions(-DPOSIX_MISTAKE -DREDEBUG)

    add_library(craneweb_dbg STATIC ${REGEX_SOURCES} ${CRANEWEB_DBG_SOURCES} ${LIBUSF_SOURCES})

    # for the custom library
    link_directories(${craneweb_BINARY_DIR}/tests)

    # and we need a custom private header
    add_custom_command(TARGET craneweb_dbg
                       PRE_BUILD
                       COMMAND ${BUILD_HEADER} craneweb_private.h ${CRANEWEB_DBG_SOURCES}
                       WORKING_DIRECTORY ${craneweb_BINARY_DIR}/tests)

    add_executable(check_stub check_stub.c)
    target_link_libraries(check_stub check)
    target_link_libraries(check_stub craneweb_dbg)
    
    add_executable(check_route_parse check_route_parse.c)
    target_link_libraries(check_route_parse check)
    target_link_libraries(check_route_parse craneweb_dbg)

    add_executable(check_route_build check_route_build.c)
    target_link_libraries(check_route_build check)
    target_link_libraries(check_route_build craneweb_dbg)
endif(ENABLE_TESTS)

