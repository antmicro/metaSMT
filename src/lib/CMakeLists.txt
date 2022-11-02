add_library(metaSMT STATIC _var_id.cpp)

target_include_directories(
  metaSMT PUBLIC $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
                 $<INSTALL_INTERFACE:include>)

target_link_directories(
  metaSMT PUBLIC $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/lib>
  $<INSTALL_INTERFACE:lib>)

target_link_libraries(metaSMT ${metaSMT_LIBS})
set_target_properties(metaSMT PROPERTIES POSITION_INDEPENDENT_CODE on)

install(
  TARGETS metaSMT
  EXPORT metaSMT
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib)
