
get_filename_component (mlsdr_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
set (mlsdr_INCLUDE_DIRS "@CONF_INCLUDE_DIRS@")

if (NOT TARGET mlsdr AND NOT mlsdr_BINARY_DIR)
	include ("${mlsdr_CMAKE_DIR}/mlsdr-targets.cmake")
endif ()

set (mlsdr_LIBRARIES mlsdr)
