project(metaSMT)
cmake_minimum_required(VERSION 3.16.0)
include(ExternalProject)

include(ProcessorCount)
ProcessorCount(JOB_COUNT)

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-std=c++17" COMPILER_SUPPORTS_CXX17)
if(COMPILER_SUPPORTS_CXX17)
  add_definitions(-std=c++17)
else()
  message(
    FATAL_ERROR
      "The compiler ${CMAKE_CXX_COMPILER} does not support -std=c++17. Please use a different C++ compiler."
  )
endif()

include(CheckIncludeFileCXX)
include(FeatureSummary)

find_package(Git REQUIRED)

list(INSERT CMAKE_PREFIX_PATH 0 "${CMAKE_SOURCE_DIR}/cmake")

# Set the default version number, but try to calculate a proper version from
# git.
set(metaSMT_VERSION 4)
include(cmake/get_git_version.cmake)

include(cmake/build_type.cmake)
include(cmake/default_compliler_flags.cmake)
include(cmake/config_file.cmake)

#
# Options to force disable certain solvers. set all solvers to "on" with good
# support and experimental or work in progress to "off"
#
option(metaSMT_USE_SWORD "try to use SWORD solver" off)
option(metaSMT_USE_Boolector "try to use Boolector solver" off)
option(metaSMT_USE_Z3 "try to use Z3 solver" off)
option(metaSMT_USE_STP "try to use STP solver" off)
option(metaSMT_USE_CVC4 "try to use CVC4 solver" off)
option(metaSMT_USE_YICES2 "try to use Yices2 solver" off)
option(metaSMT_USE_CUDD "try to use CUDD BDD package" off)
option(metaSMT_USE_MiniSat "try to use minisat SAT solver" off)
option(metaSMT_USE_picosat "try to use picosat SAT solver" off)
option(metaSMT_USE_lingeling "try to use lingeling SAT solver" off)
option(metaSMT_USE_Aiger "try to use aiger" off)
option(metaSMT_USE_CW "try to use clause writers" off)

option(metaSMT_DOC_Doxygen "try to build doxygen documentation" off)
option(metaSMT_ENABLE_TESTS "build tests" off)

set(metaSMT_CONFIG_DIR
    "share/metaSMT"
    CACHE PATH "where to install CMake and Make Config files")

if(WIN32)
  message(STATUS "Windows build: disable all solvers except z3!")
  set(metaSMT_USE_SWORD off)
  set(metaSMT_USE_Boolector off)
  set(metaSMT_USE_STP off)
  set(metaSMT_USE_CVC4 off)
  set(metaSMT_USE_YICES2 off)
  set(metaSMT_USE_CUDD off)
  set(metaSMT_USE_MiniSat off)
  set(metaSMT_USE_picosat off)
  set(metaSMT_USE_lingeling off)
  set(metaSMT_USE_Aiger off)
  set(metaSMT_USE_CW off)
endif()

if(metaSMT_USE_SWORD AND NOT APPLE)
  find_package(SWORD)
else()
  set(SWORD_FOUND 0)
endif()

function(SETUP_MiniSat)
  if(NOT MiniSat_FOUND)
    ExternalProject_Add(
      MiniSat
      PREFIX minisat
      BINARY_DIR minisat
      GIT_REPOSITORY https://github.com/niklasso/minisat
      GIT_SHALLOW TRUE
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}
      UPDATE_COMMAND ""
      PATCH_COMMAND git apply ${PROJECT_SOURCE_DIR}/minisat.patch || true
      BUILD_COMMAND cmake --build . -j${JOB_COUNT}
      TEST_COMMAND "")
    list(APPEND metaSMT_LIBS minisat)
    set(MiniSat_FOUND
        1
        PARENT_SCOPE)
  endif()
endfunction()

if(metaSMT_USE_MiniSat)
  setup_minisat()
else()
  set(MiniSat_FOUND 0)
endif()

if(metaSMT_USE_Boolector)
  if(NOT MiniSat_FOUND)
    setup_minisat()
  endif()
  ExternalProject_Add(
    Btor2Tools
    PREFIX btor2tools
    GIT_REPOSITORY https://github.com/boolector/btor2tools
    GIT_SHALLOW TRUE
    BUILD_IN_SOURCE TRUE
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}
               -DBUILD_SHARED_LIBS=OFF
    UPDATE_COMMAND ""
    BUILD_COMMAND cmake --build . -j${JOB_COUNT}
    TEST_COMMAND "")
  ExternalProject_Add(
    Boolector
    PREFIX boolector
    GIT_REPOSITORY https://github.com/Boolector/boolector
    GIT_SHALLOW TRUE
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}
               -DBtor2Tools_INCLUDE_DIR=${PROJECT_BINARY_DIR}/include
               -DBtor2Tools_LIBRARIES=${PROJECT_BINARY_DIR}/lib/libbtor2parser.a
               -DMiniSat_INCLUDE_DIR=${PROJECT_BINARY_DIR}/include
               -DMiniSat_LIBRARIES=${PROJECT_BINARY_DIR}/lib/libminisat.a
               -DBUILD_SHARED_LIBS=OFF
    UPDATE_COMMAND ""
    BUILD_COMMAND cmake --build . -j${JOB_COUNT}
    TEST_COMMAND "")
  list(APPEND metaSMT_LIBS minisat boolector btor2parser)
  list(APPEND metaSMT_AVAILABLE_QF_ABV_SOLVERS "BTOR")
  set(Boolector_FOUND 1)
else()
  set(Boolector_FOUND 0)
endif()

if(metaSMT_USE_Z3)
  ExternalProject_Add(
    Z3
    PREFIX z3
    GIT_REPOSITORY https://github.com/Z3Prover/z3
    GIT_TAG z3-4.11.2
    GIT_SHALLOW TRUE
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}
               -DZ3_BUILD_LIBZ3_SHARED=OFF -DZ3_ENABLE_EXAMPLE_TARGETS=OFF
               -DZ3_BUILD_EXECUTABLE=OFF -DZ3_BUILD_TEST_EXECUTABLES=OFF
    BUILD_COMMAND cmake --build . -j${JOB_COUNT}
    TEST_COMMAND "")
  list(APPEND metaSMT_LIBS z3)
  list(APPEND metaSMT_AVAILABLE_QF_ABV_SOLVERS "Z3")
  set(Z3_FOUND 1)
else()
  set(Z3_FOUND 0)
endif()

if(metaSMT_USE_YICES2)
  find_package(YICES2)
else()
  set(YICES2_FOUND 0)
endif()

if(metaSMT_USE_STP)
  if(NOT MiniSat_FOUND)
    setup_minisat()
  endif()
  ExternalProject_Add(
    STP
    PREFIX stp
    DEPENDS MiniSat
    GIT_REPOSITORY https://github.com/antmicro/stp
    GIT_TAG smtcomp2020-branch
    GIT_SHALLOW TRUE
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}
               -DNOCRYPTOMINISAT=ON
               -DONLY_SIMPLE=ON
               -DSTATICCOMPILE=ON
               -DMINISAT_LIBRARY=${PROJECT_BINARY_DIR}/lib/libminisat.a
               -DMINISAT_INCLUDE_DIR=${PROJECT_BINARY_DIR}/include
    UPDATE_COMMAND ""
    BUILD_COMMAND cmake --build . -j${JOB_COUNT}
    TEST_COMMAND "")
  list(APPEND metaSMT_LIBS minisat stp)
  list(APPEND metaSMT_AVAILABLE_QF_ABV_SOLVERS "STP")
  set(STP_FOUND 1)
else()
  set(STP_FOUND 0)
endif()

if(metaSMT_USE_CVC4)
  ExternalProject_Add(
    CVC4
    PREFIX cvc4
    GIT_REPOSITORY https://github.com/CVC4/CVC4-archived
    GIT_TAG 1.8
    GIT_SHALLOW TRUE
    BUILD_IN_SOURCE TRUE
    CONFIGURE_COMMAND contrib/get-antlr-3.4 && ./configure.sh --static
                      --no-static-binary --prefix=${PROJECT_BINARY_DIR}
    UPDATE_COMMAND ""
    BUILD_COMMAND make -C build -j${JOB_COUNT}
    TEST_COMMAND ""
    INSTALL_COMMAND make -C build install)
  list(APPEND metaSMT_LIBS gmp cvc4)
  list(APPEND metaSMT_AVAILABLE_QF_ABV_SOLVERS "CVC4")
  set(CVC4_FOUND 1)
else()
  set(CVC4_FOUND 0)
endif()

if(metaSMT_USE_CUDD)
  ExternalProject_Add(
    CUDD
    PREFIX cudd
    GIT_REPOSITORY https://github.com/ivmai/cudd
    GIT_TAG cudd-3.0.0
    GIT_SHALLOW TRUE
    BUILD_IN_SOURCE TRUE
    # Need to reconf as the repo has old autoconf output commited
    CONFIGURE_COMMAND autoreconf && ./configure --prefix=${PROJECT_BINARY_DIR}
                      --enable-obj
    UPDATE_COMMAND ""
    BUILD_COMMAND make -j${JOB_COUNT}
    TEST_COMMAND ""
    INSTALL_COMMAND make install)
  list(APPEND metaSMT_LIBS cudd)
  set(CUDD_FOUND 1)
else()
  set(CUDD_FOUND 0)
endif()

if(metaSMT_USE_Aiger)
  find_package(Aiger)
  list(APPEND Aiger_LIBRARIES Aiger)
else()
  set(Aiger_FOUND 0)
endif()

if(metaSMT_USE_picosat)
  find_package(PicoSAT)
endif()

if(metaSMT_USE_lingeling)
  find_package(Lingeling)
endif()

find_package(GMP QUIET)
if(NOT GMP_FOUND)
  find_library(GMP_LIBRARIES gmp PATHS ${GMP_DIR})
  find_library(GMPXX_LIBRARIES gmpxx PATHS ${GMP_DIR})
else()
  set(GMP_LIBRARIES "")
  set(GMPXX_LIBRARIES "")
endif()

if(Z3_FOUND)
  find_package(OpenMP)
  if(OPENMP_FOUND)
    set(Z3_CXXFLAGS ${OpenMP_CXX_FLAGS})
    if(UNIX)
      list(APPEND metaSMT_LIBS ${OpenMP_CXX_FLAGS})
    endif()
    message(STATUS "Use Z3 with OpenMP")
  else()
    message(STATUS "Use Z3 without OpenMP")
  endif()

  if(UNIX)
    find_package(Threads QUIET)
    find_library(RT_LIB rt)
    list(APPEND metaSMT_LIBS ${RT_LIB} ${CMAKE_THREAD_LIBS_INIT})
  endif()
endif()

if(CVC4_FOUND)
  if(GMPXX_LIBRARIES)
    list(APPEND metaSMT_LIBS ${GMPXX_LIBRARIES})
  endif()

  if(GMP_LIBRARIES)
    list(APPEND metaSMT_LIBS ${GMP_LIBRARIES})
  endif()
endif()

if(metaSMT_DOC_Doxygen)
  find_package(Doxygen)
else()
  set(Doxygen_FOUND 0)
endif()

# ##############################################################################
# Set LIBS, INCLUDES and metaSMT_AVAILABLE_QF_ABV_SOLVERS  #
# ##############################################################################

set(metaSMT_AVAILABLE_QF_ABV_SOLVERS "")
set(metaSMT_INCLUDES ${Boost_INCLUDE_DIRS})

if(SWORD_FOUND)
  config_find(SWORD ${SWORD_DIR})
  list(APPEND metaSMT_LIBS ${SWORD_LIBRARIES})
  list(APPEND metaSMT_INCLUDES ${SWORD_INCLUDE_DIR})
endif()

if(YICES2_FOUND)
  config_find(YICES2 ${YICES2_DIR})
  list(APPEND metaSMT_LIBS ${YICES2_LIBRARIES})
  list(APPEND metaSMT_INCLUDES ${YICES2_INCLUDE_DIR})
  list(APPEND metaSMT_AVAILABLE_QF_ABV_SOLVERS "YICES2")
endif()

if(Aiger_FOUND)
  config_find(Aiger ${Aiger_DIR})
  list(APPEND metaSMT_LIBS ${Aiger_LIBRARIES})
  list(APPEND metaSMT_INCLUDES ${Aiger_INCLUDE_DIR})
endif()

if(PicoSAT_FOUND)
  config_find(PicoSAT ${PicoSAT_DIR})
  list(APPEND metaSMT_LIBS ${PicoSAT_LIBRARIES})
  list(APPEND metaSMT_INCLUDES ${PicoSAT_INCLUDE_DIR})
endif()

if(Lingeling_FOUND)
  config_find(Lingeling ${Lingeling_DIR})
  list(APPEND metaSMT_LIBS ${Lingeling_LIBRARIES})
  list(APPEND metaSMT_INCLUDES ${Lingeling_INCLUDE_DIR})
endif()

# ##############################################################################
# ############ RPATH ########################################
# ##############################################################################

include(cmake/library_rpath.cmake)
add_dependent_libraries(${metaSMT_LIBS})

# ##############################################################################
# ############ subdirectories ###############################
# ##############################################################################

add_subdirectory(src)
add_subdirectory(doc)

# ##############################################################################
# ######### generate cmake config files #####################
# ##############################################################################

generate_config_files()

feature_summary(WHAT ALL)

# ##############################################################################
# CPack settings                          ##
# ##############################################################################

# CPack version numbers for release tarball name.
set(CPACK_PACKAGE_VERSION_MAJOR ${metaSMT_VERSION})

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SMT programming library")
set(CPACK_PACKAGE_VENDOR "metaSMT development team")
set(CPACK_PACKAGE_DESCRIPTION_FILE ${PROJECT_SOURCE_DIR}/README.md)
set(CPACK_GENERATOR TGZ)
set(CPACK_SOURCE_PACKAGE_FILE_NAME
    "metaSMT-${metaSMT_VERSION}"
    CACHE INTERNAL "tarball basename")
set(CPACK_SOURCE_GENERATOR TGZ TBZ2)

set(CPACK_SOURCE_IGNORE_FILES "/\\\\.git" "/deps" "/build.*/" "/root")
include(CPack)

# ##############################################################################
# ##############################################################################

# vim: ft=cmake:ts=2:sw=2:expandtab
