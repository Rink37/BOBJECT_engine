set( _OpenCV_HEADER_SEARCH_DIRS
"/usr/include"
"/usr/local/include"
"${CMAKE_SOURCE_DIR}/includes"
"C:/Program Files (x86)/opencv/build/" )
set( _OpenCV_LIB_SEARCH_DIRS
"/usr/lib"
"/usr/local/lib"
"${CMAKE_SOURCE_DIR}/lib"
"C:/Program Files (x86)/opencv/build/x64/vc16/lib" )

# Check environment for root search directory
set( _OpenCV_ENV_ROOT $ENV{OPENCV_ROOT} )
if( NOT OPENCV_ROOT AND _OpenCV_ENV_ROOT )
	set(OPENCV_ROOT ${_OpenCV_ENV_ROOT} )
endif()

# Put user specified location at beginning of search
if( OPENCV_ROOT )
	list( INSERT _OpenCV_HEADER_SEARCH_DIRS 0 "${OPENCV_ROOT}/include" )
	list( INSERT _OpenCV_LIB_SEARCH_DIRS 0 "${OPENCV_ROOT}/lib" )
endif()

# Search for the header
FIND_PATH(OPENCV_INCLUDE_DIR "opencv2/opencv.hpp"
PATHS ${_OpenCV_HEADER_SEARCH_DIRS} )

# Search for the dll
FIND_PATH(OPENCV_DLL_DIR "opencv_world4100.dll"
PATHS ${_OpenCV_LIB_SEARCH_DIRS}/.. )

# Search for the library
FIND_LIBRARY(OPENCV_LIBRARY NAMES opencv_world4100d
PATHS ${_OpenCV_LIB_SEARCH_DIRS})
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenCV DEFAULT_MSG
OPENCV_LIBRARY OPENCV_INCLUDE_DIR)

IF(OPENCV_FOUND)
    MESSAGE(STATUS "OPENCV_INCLUDE_DIR = ${OPENCV_INCLUDE_DIR}")
    MESSAGE(STATUS "OPENCV_LIBRARY = ${OPENCV_LIBRARY}")
    MESSAGE(STATUS "OPENCV_DLL_DIR = ${OPENCV_DLL_DIR}")
ENDIF(OPENCV_FOUND)