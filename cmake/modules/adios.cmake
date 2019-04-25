option(ENABLE_ADIOS "Enables the ADIOS2 writer type." ON)
if(ENABLE_ADIOS)
    message(STATUS "Installing Adios2.")
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

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_ADIOS ${LINK_TYPE}")

    # Enable ExternalProject CMake module
    include(ExternalProject)

    ## source
    set(ADIOS2_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/adios2_source)
    ## install directory
    set(ADIOS2_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/adios2)
    ## following is needed to make INTERFACE_INCLUDE_DIRECTORIES work
    set(ADIOS2_INCLUDE_DIR ${ADIOS2_INSTALL_DIR}/include)

    # Download and install zeromq
    ExternalProject_Add(
        adios2
        GIT_REPOSITORY https://github.com/ornladios/ADIOS2.git
        GIT_TAG master
        SOURCE_DIR ${ADIOS2_SOURCE_DIR}
        CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
          -DCMAKE_BUILD_TYPE=Release -DADIOS2_USE_BZip2=OFF
          -DADIOS2_USE_Fortran=OFF -DADIOS2_USE_HDF5=OFF 
          -DADIOS2_USE_Python=OFF -DADIOS2_USE_SST=OFF 
          -DADIOS2_USE_SZ=OFF -DADIOS2_USE_SysVShMem=OFF 
          -DADIOS2_USE_ZFP=OFF -DADIOS2_USE_ZeroMQ=OFF 
          -DADIOS2_BUILD_EXAMPLE=OFF -DADIOS2_BUILD_TESTING=OFF
          -DMPI_GUESS_LIBRARY_NAME=${MPI_GUESS_LIBRARY_NAME}
          -DCMAKE_INSTALL_PREFIX=${ADIOS2_INSTALL_DIR}
    )

    ExternalProject_get_property(adios2 BINARY_DIR)
    # following lines are some hardcore hacking to appease stupid cmake
    # taken from https://stackoverflow.com/questions/45516209/cmake-how-to-use-interface-include-directories-with-externalproject
    # also cf. https://www.youtube.com/watch?v=tbDw7-5701k
    file(MAKE_DIRECTORY ${ADIOS2_INCLUDE_DIR})

    # Create a libadios2 target to be used as a dependency by the program
    add_library(libadios2 IMPORTED SHARED GLOBAL)
    add_dependencies(libadios2 adios2)
#    get_target_property(LIB_TYPE libadios2 TYPE)
#    if(LIB_TYPE STREQUAL "SHARED")
#        set(LIB_SUFFIX "so")
#    else()
#        set(LIB_SUFFIX "a")
#    endif()
    include(GNUInstallDirs)
    set_target_properties(libadios2 PROPERTIES
        IMPORTED_LOCATION "${BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/libadios2.so"
        INTERFACE_INCLUDE_DIRECTORIES "${ADIOS2_INCLUDE_DIR}"
    )
else()
    # sad we have to do this, but cmake wont allow to create empty targets for linking
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/tmp/adios2_dummy.c)              # create an empty source file
    add_library(libadios2 ${CMAKE_CURRENT_BINARY_DIR}/tmp/adios2_dummy.c)   # create a dummy target from that empty file
endif()
