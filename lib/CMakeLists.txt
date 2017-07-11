cmake_minimum_required (VERSION 2.8)
project (mlsdr)

set (mlsdr_VERSION 0.1)

set (INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set (INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set (INSTALL_INCLUDE_DIR include CACHE PATH
	"Installation directory for header files")
set (INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH
	"Installation directory for CMake files")

set (CMAKE_C_STANDARD 11)

foreach (p LIB BIN INCLUDE CMAKE)
	set (var INSTALL_${p}_DIR)
	if (NOT IS_ABSOLUTE "${${var}}")
		set (${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
	endif ()
	unset (var)
endforeach ()

find_package (LibFTDI1 REQUIRED)
find_package (PkgConfig REQUIRED)
find_package (Volk REQUIRED)

pkg_search_module (LIBUSB REQUIRED libusb libusb-1.0)

if (CMAKE_BUILD_TYPE MATCHES Debug)
	message (STATUS "Building in Debug mode, using ASAN")
	link_libraries (asan)
	add_definitions (-lasan -fsanitize=address -fno-omit-frame-pointer)
endif ()

add_definitions (-O3 -ggdb3)
add_definitions (-Wall -Wno-unused-function -Wno-unused-variable)
include_directories (${PROJECT_SOURCE_DIR} ${LIBFTDI_INCLUDE_DIRS}
	${VOLK_INCLUDE_DIRS})

add_library (mlsdr SHARED mlsdr.c ring.c regmap.c r820t.c utils.c iqconv.c)
target_link_libraries (mlsdr bsd ${LIBUSB_LIBRARIES} ${LIBFTDI_LIBRARY}
	${VOLK_LIBRARIES})

add_executable (mlsdr_sdr mlsdr_sdr.c)
target_link_libraries (mlsdr_sdr mlsdr pthread)

add_executable (mlsdr_list mlsdr_list.c)
target_link_libraries (mlsdr_list mlsdr)

set_target_properties (mlsdr PROPERTIES
	PUBLIC_HEADER "mlsdr.h;r820t.h;regmap.h")

install (TARGETS mlsdr
	EXPORT mlsdr-targets
	RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
	LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
	PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/${PROJECT_NAME}"
		COMPONENT dev)

install (TARGETS mlsdr_sdr mlsdr_list
	EXPORT mlsdr-targets
	RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin)

# make uninstall target
configure_file (
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY
)
add_custom_target (uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

# Package generating magic

export (TARGETS mlsdr
	FILE "${PROJECT_BINARY_DIR}/mlsdr-targets.cmake")

export (PACKAGE mlsdr)
include (CMakePackageConfigHelpers)

file (RELATIVE_PATH REL_INCLUDE_DIR "${INSTALL_CMAKE_DIR}"
	"${INSTALL_INCLUDE_DIR}")
set (CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
configure_file (cmake/mlsdr-config.cmake
	"${PROJECT_BINARY_DIR}/mlsdr-config.cmake" @ONLY)
set (CONF_INCLUDE_DIRS "\${mlsdr_CMAKE_DIR}/${REL_INCLUDE_DIR}")
configure_file (cmake/mlsdr-config.cmake
	"${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/mlsdr-config.cmake" @ONLY)

write_basic_package_version_file (
	"${CMAKE_CURRENT_BINARY_DIR}/mlsdr-config-version.cmake"
	VERSION ${mlsdr_VERSION}
	COMPATIBILITY AnyNewerVersion
)

install (FILES
	"${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/mlsdr-config.cmake"
	"${PROJECT_BINARY_DIR}/mlsdr-config-version.cmake"
	DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)

install (EXPORT mlsdr-targets DESTINATION
	"${INSTALL_CMAKE_DIR}" COMPONENT dev)
