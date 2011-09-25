

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
    include_directories(${REGEX_DIR})

    # we need a custom debug build
    # base
    add_definitions(-DHAVE_CONFIG_H)
    add_definitions(-DCRW_PRIVATE=extern)
    add_definitions(-DCRW_DEBUG=1)
    # regex
    add_definitions(-DPOSIX_MISTAKE -DREDEBUG)

    add_library(craneweb_dbg STATIC ${REGEX_SOURCES} ${CRANEWEB_DBG_SOURCES})

    # for the custom library
    link_directories(${craneweb_BINARY_DIR}/tests)

    # and we need a custom private header
    add_custom_command(TARGET craneweb_dbg
                       PRE_BUILD
                       COMMAND ${BUILD_HEADER} craneweb_private.h ${CRANEWEB_DBG_SOURCES}
                       WORKING_DIRECTORY ${craneweb_BINARY_DIR}/tests)

    add_executable(check_template check_template.c)
    target_link_libraries(check_template check)
    target_link_libraries(check_template craneweb_dbg)
    
    add_executable(check_route check_route.c)
    target_link_libraries(check_route check)
    target_link_libraries(check_route craneweb_dbg)
endif(ENABLE_TESTS)

