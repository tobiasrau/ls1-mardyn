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
    ## following is needed to make INTERFACE_INCLUDE_DIRECTORIES work
    set(ZMQ_INCLUDE_DIR ${ZMQ_SOURCE_DIR}/include)

    # Download and install zeromq
    ExternalProject_Add(
        zmq
        GIT_REPOSITORY https://github.com/zeromq/libzmq.git
        GIT_TAG master
        SOURCE_DIR ${ZMQ_SOURCE_DIR}
        BINARY_DIR ${ZMQ_BINARY_DIR}
        INSTALL_COMMAND ""    
        CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    )

    # hardcore hacking to appease stupid cmake
    # taken from https://stackoverflow.com/questions/45516209/cmake-how-to-use-interface-include-directories-with-externalproject
    # also cf. https://www.youtube.com/watch?v=tbDw7-5701k
    file(MAKE_DIRECTORY ${ZMQ_INCLUDE_DIR})

    # Create a libzmq target to be used as a dependency by the program
    add_library(libzmq IMPORTED STATIC GLOBAL)
    add_dependencies(libzmq zmq)
    set_target_properties(libzmq PROPERTIES
        IMPORTED_LOCATION "${ZMQ_BINARY_DIR}/lib/libzmq.so"
        INTERFACE_INCLUDE_DIRECTORIES "${ZMQ_INCLUDE_DIR}"
    )
else()
    # sad we have to do this, but cmake wont allow to create empty targets for linking
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/zmq_dummy.c)           # create an empty source file
    add_library(libzmq ${CMAKE_CURRENT_BINARY_DIR}/zmq_dummy.c)   # create a dummy target from that empty file
    message(STATUS "Insitu Disabled")
endif()