
FIND_PATH(LIBCHECK_INCLUDE_DIR check.h PATHS /usr/include/ /usr/local/include/)

FIND_LIBRARY(LIBCHECK_LIBRARY NAMES check PATH /usr/lib /usr/local/lib) 

IF(LIBCHECK_INCLUDE_DIR AND LIBCHECK_LIBRARY)
   SET(LIBCHECK_FOUND TRUE)
ENDIF(LIBCHECK_INCLUDE_DIR AND LIBCHECK_LIBRARY)

IF(LIBCHECK_FOUND)
   IF(NOT LIBCHECK_FIND_QUIETLY)
      MESSAGE(STATUS "Found CHECK: ${LIBCHECK_LIBRARY}")
   ENDIF(NOT LIBCHECK_FIND_QUIETLY)
ELSE(LIBCHECK_FOUND)
   IF(LIBCHECK_FIND_REQUIRED)
      MESSAGE(SEND_ERROR "Could not find CHECK - testsuite will NOT be avalaible")
   ENDIF(LIBCHECK_FIND_REQUIRED)
ENDIF(LIBCHECK_FOUND)

