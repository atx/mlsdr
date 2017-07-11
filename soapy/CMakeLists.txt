cmake_minimum_required (VERSION 2.6)
project (soapymlsdr)

find_package (SoapySDR REQUIRED)
find_package (mlsdr REQUIRED)

add_definitions (-std=gnu++11)
add_definitions (-Wno-unused-parameter)

SOAPY_SDR_MODULE_UTIL(
	TARGET mlsdrSupport
	SOURCES
		Registration.cpp
		Settings.cpp
		Streaming.cpp
	LIBRARIES
		mlsdr
)

include_directories (${CMAKE_CURRENT_SOURCE_DIR} ${mlsdr_INCLUDE_DIRS})
