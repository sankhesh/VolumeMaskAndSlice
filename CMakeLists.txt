cmake_minimum_required(VERSION 2.8)

PROJECT(VolumeMaskAndSlice)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

set (${PROJECT_NAME}_SRCS
  ${PROJECT_NAME}.cxx
  vtkImageMapToRGBA.cxx
  vtkImageMapToRGBA.h
  )

set (${PROJECT_NAME}2_SRCS
  ${PROJECT_NAME}2.cxx
  )

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
  ${${PROJECT_NAME}_SRCS})

add_executable (${PROJECT_NAME}2 MACOSX_BUNDLE
  ${${PROJECT_NAME}2_SRCS})

if(VTK_LIBRARIES)
  target_link_libraries(${PROJECT_NAME} ${VTK_LIBRARIES})
  target_link_libraries(${PROJECT_NAME}2 ${VTK_LIBRARIES})
else()
  target_link_libraries(${PROJECT_NAME} vtkHybrid vtkWidgets)
  target_link_libraries(${PROJECT_NAME}2 vtkHybrid vtkWidgets)
endif()

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/Data
  COMMAND ${CMAKE_COMMAND}
  ARGS -E copy_directory ${CMAKE_SOURCE_DIR}/Data ${CMAKE_BINARY_DIR}/Data
  COMMENT "Copying Data directory"
  MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/Data
  )
add_custom_target(copy_data ALL DEPENDS "${CMAKE_BINARY_DIR}/Data")
add_dependencies(${PROJECT_NAME} copy_data)
add_dependencies(${PROJECT_NAME}2 copy_data)
