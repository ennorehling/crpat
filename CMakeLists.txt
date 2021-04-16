cmake_minimum_required(VERSION 2.8...3.13)
project (crpat C)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

FILE (GLOB LIB_HDR *.h)
ADD_LIBRARY (crpat
	crpat.c 
	${LIB_HDR})

#ADD_EXECUTABLE (test_crpat tests.c crpat.test.c CuTest.c)
#TARGET_LINK_LIBRARIES (test_crpat crpat)
#ADD_TEST (crpat test_crpat)

SET (CRPAT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "crpat headers")
SET (CRPAT_LIBRARIES crpat CACHE INTERNAL "crpat libraries")

ADD_EXECUTABLE (crprint example/crprint.c)
TARGET_LINK_LIBRARIES (crprint crpat)
