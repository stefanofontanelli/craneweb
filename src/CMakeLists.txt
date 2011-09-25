
set(CRANEWEB_SOURCES craneweb.c)

set(REGEX_DIR ${craneweb_SOURCE_DIR}/deps/regex-3.8a)
if(ENABLE_BUILTIN_REGEX)
    set(REGEX_SOURCES ${REGEX_DIR}/regcomp.c ${REGEX_DIR}/regexec.c ${REGEX_DIR}/regerror.c ${REGEX_DIR}/regfree.c)
endif(ENABLE_BUILTIN_REGEX)

set(MONGOOSE_DIR ${craneweb_SOURCE_DIR}/deps/mongoose)
if(ENABLE_BUILTIN_MONGOOSE)
    set(MONGOOSE_SOURCES ${MONGOOSE_DIR}/mongoose.c)
endif(ENABLE_BUILTIN_MONGOOSE)


# base
add_definitions(-DHAVE_CONFIG_H)
add_definitions(-DCRW_PRIVATE=static)
# regex
add_definitions(-DPOSIX_MISTAKE -DREDEBUG)
# mongoose (FIXME)
#add_definitions(-DNO_CGI)


include_directories(${craneweb_BINARY_DIR})
include_directories(${craneweb_SOURCE_DIR})
include_directories(${craneweb_SOURCE_DIR}/src)
include_directories(${craneweb_SOURCE_DIR}/deps/)
include_directories(${REGEX_DIR})
include_directories(${MONGOOSE_DIR})

add_library(craneweb_s STATIC ${REGEX_SOURCES} ${MONGOOSE_SOURCES} ${CRANEWEB_SOURCES})
add_library(craneweb SHARED ${REGEX_SOURCES} ${MONGOOSE_SOURCES} ${CRANEWEB_SOURCES})

# mongoose
target_link_libraries(craneweb_s pthread)
target_link_libraries(craneweb_s dl)
target_link_libraries(craneweb pthread)
target_link_libraries(craneweb dl)

