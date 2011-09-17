
set(CRANEWEB_SOURCES craneweb.c)

set(REGEX_DIR ${craneweb_SOURCE_DIR}/deps/regex-3.8a)
set(REGEX_SOURCES ${REGEX_DIR}/regcomp.c ${REGEX_DIR}/regexec.c ${REGEX_DIR}/regerror.c ${REGEX_DIR}/regfree.c)

set(MONGOOSE_DIR ${craneweb_SOURCE_DIR}/deps/mongoose)
set(MONGOOSE_SOURCES ${MONGOOSE_DIR}/mongoose.c)

# base
add_definitions(-DHAVE_CONFIG_H)
# regex
add_definitions(-DPOSIX_MISTAKE -DREDEBUG)
# mongoose (FIXME)
#add_definitions(-DNO_CGI)
if(CMAKE_COMPILER_IS_GNUCC)
    add_definitions(-std=c99 -W -Wall -pedantic)
endif(CMAKE_COMPILER_IS_GNUCC)


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

