cmake_minimum_required(VERSION 2.8)
PROJECT(showreg)
SET(CMAKE_BUILD_TYPE Release)
#SET(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_VERBOSE_MAKEFILE false)
ADD_DEFINITIONS(-Wall -Wextra -Wshadow -Wconversion -Wno-unused-parameter -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)

ADD_EXECUTABLE(showreg
    main.cpp
    XmlReader.cpp
)
TARGET_LINK_LIBRARIES(showreg rt)

