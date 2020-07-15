cmake_minimum_required(VERSION 3.17.2)

if (${CMAKE_GENERATOR} MATCHES "^Visual Studio")
   if (${CMAKE_GENERATOR_PLATFORM} STREQUAL "x64")
      set(
         CMAKE_PREFIX_PATH
         ${CMAKE_PREFIX_PATH}
         "${VCPKG_ROOT}/installed/x64-windows"
         "${VCPKG_ROOT}/installed/x64-windows/debug")
   elseif (${CMAKE_GENERATOR_PLATFORM} STREQUAL "Win32")
      set(
         CMAKE_PREFIX_PATH
         ${CMAKE_PREFIX_PATH}
         "${VCPKG_ROOT}/installed/x86-windows"
         "${VCPKG_ROOT}/installed/x86-windows/debug")
   else ( )
      message(
         FATAL_ERROR
         "Platform architecture not supported!")
   endif ( )
else ( )
   message(
      FATAL_ERROR
      "${CMAKE_GENERATOR} is not supported!")
endif ( )

set(
   CMAKE_AUTOMOC
   yes)

set(
   CMAKE_CXX_STANDARD
   17)

set(
   proj_name
   qt-mtgl-ctx-sharing)

project(
   ${proj_name})

add_executable(
   ${proj_name}
   color-buffer-data.h
   main.cpp
   osg-gc-wrapper.cpp
   osg-gc-wrapper.h
   osg-view.cpp
   osg-view.h
   qt-gl-view.cpp
   qt-gl-view.h
   render-thread.cpp
   render-thread.h)

find_package(
   Qt5 REQUIRED
   COMPONENTS
   Core
   Gui
   Widgets)

target_link_libraries(
   ${proj_name}
   PRIVATE
   Qt5::Core
   Qt5::Gui
   Qt5::Widgets)

find_package(
   OpenSceneGraph REQUIRED
   COMPONENTS
   osgDB
   osgViewer
   osgUtil)

target_include_directories(
   ${proj_name}
   PRIVATE
   ${OPENSCENEGRAPH_INCLUDE_DIRS})
target_link_libraries(
   ${proj_name}
   PRIVATE
   ${OPENSCENEGRAPH_LIBRARIES})

find_package(
   OpenGL REQUIRED)

target_link_libraries(
   ${proj_name}
   PRIVATE
   OpenGL::GL)
