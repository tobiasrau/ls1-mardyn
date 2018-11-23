# mpi
option(ENABLE_INSITU "Enables Insitu Plugin, to be used in conjunction with Megamol" OFF)
if(ENABLE_INSITU)
message(STATUS "Insitu Enabled")
message(STATUS "Installing ZMQ.")
    ## set linkage for hazelhen
    if($ENV{CRAYPE_LINK_TYPE})
        set(LINK_TYPE $ENV{CRAYPE_LINK_TYPE})
	if (${LINK_TYPE} STREQUAL "dynamic")
	    set(LINK_TYPE -${LINK_TYPE})
            message(STATUS "Enabling dynamic linking for CRAY-XC40")
        else()
            message(STATUS "CRAYPE_LINK_TYPE was set, but not to dynamic, this might break things.")
        endif()
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_INSITU ${LINK_TYPE}")

    # Enable ExternalProject CMake module
    include(ExternalProject)

    set(ZMQ_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/zeromq)
    ## following is needed to make INTERFACE_INCLUDE_DIRECTORIES work
    set(ZMQ_INCLUDE_DIR ${ZMQ_SOURCE_DIR}/include)

    # Download and install zeromq
    ExternalProject_Add(
        zmq
        GIT_REPOSITORY https://github.com/zeromq/libzmq.git
        GIT_TAG master
        SOURCE_DIR ${ZMQ_SOURCE_DIR}
        INSTALL_COMMAND ""    
        CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    )

    ExternalProject_get_property(zmq BINARY_DIR)
    # following line hardcore hacking to appease stupid cmake
    # taken from https://stackoverflow.com/questions/45516209/cmake-how-to-use-interface-include-directories-with-externalproject
    # also cf. https://www.youtube.com/watch?v=tbDw7-5701k
    file(MAKE_DIRECTORY ${ZMQ_INCLUDE_DIR})

    # Create a libzmq target to be used as a dependency by the program
    add_library(libzmq IMPORTED SHARED GLOBAL)
    add_dependencies(libzmq zmq)
#    get_target_property(LIB_TYPE libzmq TYPE)
#    if(LIB_TYPE STREQUAL "SHARED")
#        set(LIB_SUFFIX "so")
#    else()
#        set(LIB_SUFFIX "a")
#    endif()
    set_target_properties(libzmq PROPERTIES
        IMPORTED_LOCATION "${BINARY_DIR}/lib/libzmq.so"
        INTERFACE_INCLUDE_DIRECTORIES "${ZMQ_INCLUDE_DIR}"
    )
else()
    # sad we have to do this, but cmake wont allow to create empty targets for linking
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/tmp/zmq_dummy.c)           # create an empty source file
    add_library(libzmq ${CMAKE_CURRENT_BINARY_DIR}/tmp/zmq_dummy.c)   # create a dummy target from that empty file
    message(STATUS "Insitu Disabled")
endif()
