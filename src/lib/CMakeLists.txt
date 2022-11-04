add_library(${PROJECT_NAME} STATIC _var_id.cpp)

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
                         $<INSTALL_INTERFACE:include>)
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                 ${PROJECT_BINARY_DIR}/lib)

target_link_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/lib>
  $<INSTALL_INTERFACE:lib>)

target_link_libraries(${PROJECT_NAME} ${${PROJECT_NAME}_LIBS})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE on)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib)
