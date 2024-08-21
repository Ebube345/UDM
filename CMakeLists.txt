cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(UDM LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (include external/asio-latest/include)

file(GLOB_RECURSE SRC_FILES ${include}/*.h ${include}/*.hpp)

set (	SRCS1
	udmuser_cl.cpp
	${SRC_FILES}
	)

set (	SRCS2
	udmuser_srv.cpp
	${SRC_FILES})

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if(UNIX)
	message(STATUS "PLATFORM IS LINUX")
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)
	add_executable(cl ${SRCS1})
	add_executable(srv ${SRCS2})
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_OPTS}")
	target_link_libraries(cl PRIVATE Threads::Threads)
	target_link_libraries(srv PRIVATE Threads::Threads)
endif(UNIX)

if(WIN32)
	message(STATUS "PLATFORM IS WINDOWS")
	
	add_executable(cl WIN32 ${SRCS1})
	add_executable(srv WIN32 ${SRCS2})
endif(WIN32)

if(APPLE)
	message(STATUS "PLATFORM IS LINUX")
	
	add_executable(cl MACOSX_BUNDLE ${SRCS1})
	add_executable(srv MACOSX_BUNDLE ${SRCS2})
endif(APPLE)

set(CMAKE_INSTALL_RPATH "${CMAKE_SOURCE_DIR}")

target_include_directories(cl PRIVATE ${cmake_source_dir})
target_include_directories(srv PRIVATE ${cmake_source_dir})

target_include_directories(cl PRIVATE ${include} PRIVATE "src/include")
target_include_directories(srv PRIVATE ${include} PRIVATE "src/include")

