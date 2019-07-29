# ---- Library ----

add_library(QcITSTask src/ITSTask.cxx)

target_sources(QcITSTask PRIVATE)

target_include_directories(
  QcITSTask
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${O2_ROOT}/include/GPU)

target_link_libraries(QcITSTask PUBLIC QualityControl)

install(TARGETS QcITSTask
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- ROOT dictionary ----

add_root_dictionary(QcITSTask
                    HEADERS include/ITS/ITSTask.h
                    LINKDEF include/ITS/LinkDef.h
                    BASENAME QcITSTask)

# ---- Test(s) ----

add_executable(testQcITSTask test/testITS.cxx)
target_link_libraries(testQcITSTask PRIVATE QcITSTask Boost::unit_test_framework O2::ITSMFTReconstruction O2::ITSBase O2::DetectorsBase QualityControl)
add_test(NAME testQcITSTask COMMAND testQcITS)
set_tests_properties(testQcITSTask PROPERTIES TIMEOUT 60)
install(
  TARGETS testQcITSTask
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- Executables ----

set(EXE_SRCS src/runITS.cxx)

set(EXE_NAMES o2-qc-run-its)

# These were the original names before the convention changed. We will get rid
# of them but for the time being we want to create symlinks to avoid confusion.
set(EXE_OLD_NAMES qcRunIts)

list(LENGTH EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET EXE_SRCS ${i} src)
  list(GET EXE_NAMES ${i} name)
  list(GET EXE_OLD_NAMES ${i} oldname)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE QualityControl CURL::libcurl O2::ITSQCDataReaderWorkflow O2::DetectorsBase)
  install_symlink(${name} ${CMAKE_INSTALL_FULL_BINDIR}/${oldname})
endforeach()

install(
  TARGETS ${EXE_NAMES}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- Extra scripts ----

install(FILES its.json DESTINATION etc)

get_property(dirs
             DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
