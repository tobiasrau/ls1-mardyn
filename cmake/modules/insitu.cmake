# mpi
option(ENABLE_INSITU "Enables Insitu Plugin, to be used in conjunction with Megamol" OFF)
if(ENABLE_INSITU)
message(STATUS "Insitu Enabled")
message(STATUS "Installing ZMQ.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_INSITU")

    # Enable ExternalProject CMake module
    include(ExternalProject)

    set(ZMQ_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/zeromq)
    set(ZMQ_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/libs/zeromq)

    # Download and install zeromq
    ExternalProject_Add(
        lz4
        GIT_REPOSITORY https://github.com/zeromq/libzmq.git
        GIT_TAG master
        SOURCE_DIR ${ZMQ_SOURCE_DIR}
        BINARY_DIR ${ZMQ_BINARY_DIR}
        INSTALL_COMMAND ""    
    )

    # Get lz4 source and binary directories from CMake project
    ExternalProject_Get_Property(zmq source_dir binary_dir)

    # Create a liblz4 target to be used as a dependency by the program
    add_library(libzmq IMPORTED STATIC GLOBAL)
    add_dependencies(libzmq zmq)

    include_directories(
        ${LZ4_SOURCE_DIR}/lib
    )
    set(ZMQ_LIB ${ZMQ_BINARY_DIR}/libzmq.so)
else()
    set(ZMQ_LIB "")
    message(STATUS "Insitu Disabled")
endif()
