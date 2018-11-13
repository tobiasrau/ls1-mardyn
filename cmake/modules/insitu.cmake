# mpi
option(ENABLE_INSITU "Enables Insitu Plugin, to be used in conjunction with Megamol" OFF)
if(ENABLE_INSITU)
    message(STATUS "Insitu Enabled")
else()
    message(STATUS "Insitu Disabled")
endif()
