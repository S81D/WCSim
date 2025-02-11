## Based on CMakeLists.txt from AnaEx02 in examples/extended of Geant4.9.6 ##
#----------------------------------------------------------------------------
# Make CMAKE use g++ on PATH not cc in /usr/bin (which we have no control over version of)
# note: must come BEFORE 'project' line
find_program(CMAKE_C_COMPILER NAMES $ENV{CC} gcc PATHS ENV PATH NO_DEFAULT_PATH)
find_program(CMAKE_CXX_COMPILER NAMES $ENV{CXX} g++ PATHS ENV PATH NO_DEFAULT_PATH)

# Setup the project
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(WCSim)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -fdiagnostics-color=always -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fdiagnostics-color=always -std=c++1y") #-fmax-errors=1
# NOTE: Geant4.10.2 changes the way VisualizationManager is instantiated - requires a verbosity argument.  
# To handle the two types of instantiation: this env variable needs to be defined for geant versions > 10.1:
if(DEFINED ENV{GEANT4_10_2})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGEANT4_10_2")
endif()
if(DEFINED ENV{NO_GENIE})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_GENIE")
  message(STATUS "GENIE features disabled.")
else()
  message(STATUS "GENIE features enabled.")
endif()
# pass sourcefile directory
add_definitions("-DWCSIMDIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
#----------------------------------------------------------------------------
# Find ROOT (required package) in CONFIG mode. Looking for ROOTConfig.cmake.
# Crucial for loading the proper definitions!
#
if(DEFINED ENV{ROOT_PATH})
  list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
  #message(STATUS "looking for root in ${CMAKE_MODULE_PATH}")
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROOT_PATH})
  message(STATUS "looking for root in ${CMAKE_MODULE_PATH}")
endif()
find_package(ROOT CONFIG REQUIRED )
#find_package(ROOT MODULE REQUIRED)
if(NOT ROOT_FOUND)
  message(STATUS "ROOT package not found.")  
  return()
else()
  message(STATUS "found ROOT at: ${ROOT_USE_FILE}")
endif()

## Load macros: need to compile ROOT through cmake first and execute bin/thisroot.sh
include(${ROOT_USE_FILE})
if(NOT DEFINED ENV{NO_GENIE})
#----------------------------------------------------------------------------
# GENIE stuff
#

#find_package(CLHEP REQUIRED)

set(_root_internal_libraries
    # for GENIE
    Geom
    EGPythia6
    GenVector
)
set(Root_LIBRARIES ${_root_internal_libraries})

link_directories($ENV{ROOTSYS}/lib)

include_directories($ENV{GENIE}/include/GENIE)
link_directories($ENV{GENIE}/lib)

set(_genie_internal_libraries
    GFwAlg GFwUtl GPhDfrcEG GPhIBDXS GPhQELXS GFwEG GPhAMNGEG GPhDfrcXS GPhMEL GPhResEG GFwGHEP GPhAMNGXS GPhDISEG GPhMNucEG GPhResXS GFwInt GPhChmXS GPhDISXS GPhMNucXS GPhStrEG GFwMsg GPhCmn GPhGlwResEG GPhNuclSt GPhStrXS GFwNtp GPhCohEG GPhGlwResXS GPhNuElEG GPhXSIg GFwNum GPhCohXS GPhHadnz GPhNuElXS GTlFlx GFwParDat GPhDcy GPhHadTransp GPhPDF GTlGeo GFwReg GPhDeEx GPhIBDEG GPhQELEG
)

include_directories($ENV{LOG4CPP_FQ_DIR}/include)
link_directories($ENV{LOG4CPP_FQ_DIR}/lib)
set(_log4cpp_internal_libraries log4cpp)

include_directories($ENV{XERCESCROOT}/include)
link_directories($ENV{XERCESCROOT}/lib)
link_directories($ENV{LIBXML2ROOT}/lib)
set(_xerces_internal_libraries xerces-c xml2)

#include_directories($ENV{ROOTSYS}/include)
link_directories($ENV{ROOTSYS}/lib)
#include_directories($ENV{GEANT4_FQ_DIR}/include)

set(Genie_LIBRARIES ${_genie_internal_libraries} ${_log4cpp_internal_libraries} ${_xerces_internal_libraries} )
endif()
#----------------------------------------------------------------------------
#----------------------------------------------------------------------------
# Locate sources and headers for this project
# ## NOT NEEDED FOR DICT
message(STATUS "using Geant4_INCLUDE_DIR: ${Geant4_INCLUDE_DIR}")
include_directories(${PROJECT_SOURCE_DIR}/include 
                    ${PROJECT_SOURCE_DIR}/../shared/include 
                    ${Geant4_INCLUDE_DIR}
                    ${ROOT_INCLUDE_DIR})

#----------------------------------------------------------------------------
# Add libraries: need to compile the Dict before linking WCSim !!
# in standard makefile, need to make rootcint anyway before standard make
#

## WCSimRootDict.cc regeneration by rootcint
## Use ROOT 5.34.32 as some issues with PARSE_ARGUMENTS were found in older ROOT versions (ROOT 5.34.11)
ROOT_GENERATE_DICTIONARY(${CMAKE_CURRENT_SOURCE_DIR}/src/WCSimRootDict
${CMAKE_CURRENT_SOURCE_DIR}/include/WCSimRootEvent.hh
${CMAKE_CURRENT_SOURCE_DIR}/include/WCSimRootGeom.hh
${CMAKE_CURRENT_SOURCE_DIR}/include/WCSimPmtInfo.hh 
${CMAKE_CURRENT_SOURCE_DIR}/include/WCSimLAPPDInfo.hh
${CMAKE_CURRENT_SOURCE_DIR}/include/WCSimLAPPDpulse.hh
${CMAKE_CURRENT_SOURCE_DIR}/include/WCSimLAPPDpulseCluster.hh
${CMAKE_CURRENT_SOURCE_DIR}/include/WCSimRootOptions.hh
${CMAKE_CURRENT_SOURCE_DIR}/include/WCSimEnumerations.hh
LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/include/WCSimRootLinkDef.hh
MODULE WCSimRoot
)

## Crucial for reading ROOT classes: make shared object library
add_library(WCSimRoot SHARED ./src/WCSimRootEvent.cc ./src/WCSimRootGeom.cc
./src/WCSimPmtInfo.cc ./src/WCSimEnumerations.cc ./src/WCSimRootDict.cxx
./src/WCSimLAPPDInfo.cc ./src/WCSimLAPPDpulse.cc
./src/WCSimLAPPDpulseCluster.cc ./src/WCSimRootOptions.cc 
)
target_link_libraries(WCSimRoot  ${ROOT_LIBRARIES})

#----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default
# You can set WITH_GEANT4_UIVIS to OFF via the command line or ccmake/cmake-gui
# to build a batch mode only executable
#
if(DEFINED ENV{GEANT4_PATH})
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} $ENV{GEANT4_PATH})
  message(STATUS "Looking for GEANT4 in = ${CMAKE_PREFIX_PATH}")
endif()
option(WITH_GEANT4_UIVIS "Build example with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

if(NOT Geant4_FOUND)
  message(STATUS "Geant4 package not found.")
else()
  message(STATUS "found Geant4")
endif()

#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
# Setup include directory for this project
#
message(STATUS "using Geant4: ${Geant4_USE_FILE}")
include(${Geant4_USE_FILE})  ## NOT needed for Dict

#----------------------------------------------------------------------------
# CADMESH install location for reading CAD STL files
if(DEFINED ENV{ENABLE_CADMESH})
  
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} $ENV{CADMESH_CMAKE_DIR})
  message(STATUS "Looking for cadmesh in = ${CMAKE_PREFIX_PATH}")
  find_package(cadmesh)
  include_directories($ENV{CADMESH_INCLUDE_DIR})
  
  if(NOT cadmesh_FOUND)
    message(STATUS "!!! cadmesh package not found !!!")
  else()
    message(STATUS "CADMESH STL file reading features enabled.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_CADMESH")
  endif()

else()
  message(STATUS "CADMESH disabled.")
endif()

#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)


#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(WCSim WCSim.cc ${sources} ${headers})
target_link_libraries(WCSim ${Geant4_LIBRARIES} ${ROOT_LIBRARIES} ${Genie_LIBRARIES} ${Root_LIBRARIES} WCSimRoot Tree ${cadmesh_LIBRARIES})



#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build WCSim. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
set(WCSIM_SCRIPTS
  macros/jobOptions.mac
  WCSim.mac
  macros/daq.mac
  macros/annie_daq.mac
  macros/visOGLSX.mac
  macros/visRayTracer.mac
  macros/tuning_parameters.mac
  macros/primaries_directory.mac
  macros/setRandomParameters.mac
  pulsecharacteristics.root
  PHASE2_INNER_STRUCTURE.gdml
  geniedirectory.txt
  dirtdirectory.txt
  WCSimVersion.txt
  PMTPositions_Scan_Glass.txt
  PMTPositions_Scan.txt
  pmt_qe.txt
  )

foreach(_script ${WCSIM_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()

set(MRD_SCRIPTS
   mrdmodule.txt
   mrdposition.txt
   mrd-align.txt
   mrdnoisehit.txt
   )

foreach(_script ${MRD_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/src/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
  )
endforeach()

# dunno why but on sl7 we seem to need to copy these manually?
configure_file(
  ${PROJECT_SOURCE_DIR}/libWCSimRoot.rootmap
  ${PROJECT_BINARY_DIR}/libWCSimRoot.rootmap
  COPYONLY
)

# this one also needs renaming
configure_file(
  ${PROJECT_SOURCE_DIR}/WCSimRootDict_rdict.pcm
  ${PROJECT_BINARY_DIR}/libWCSimRoot_rdict.pcm
  COPYONLY
)

#----------------------------------------------------------------------------
# For internal Geant4 use - but has no effect if you build this
# example standalone
#
#add_custom_target(WCSim DEPENDS WCSim)

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
#install(TARGETS WCSim DESTINATION bin)


