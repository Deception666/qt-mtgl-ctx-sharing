cmake_minimum_required(VERSION 3.16.0)

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

set(
   CMAKE_C_FLAGS
   "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /MP")
set(
   CMAKE_CXX_FLAGS
   "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /MP")

try_compile(
   _has_cxx_structured_bindings
   "${CMAKE_BINARY_DIR}"
   "${CMAKE_SOURCE_DIR}/cmake/structured-bindings.cpp"
   CMAKE_FLAGS ${CMAKE_CXX_FLAGS})
try_compile(
   _has_cxx_std_map_extract
   "${CMAKE_BINARY_DIR}"
   "${CMAKE_SOURCE_DIR}/cmake/map-extract.cpp"
   CMAKE_FLAGS ${CMAKE_CXX_FLAGS})
try_compile(
   _has_cxx_std_shared_ptr_weak_type
   "${CMAKE_BINARY_DIR}"
   "${CMAKE_SOURCE_DIR}/cmake/shared-ptr-weak-type.cpp"
   CMAKE_FLAGS ${CMAKE_CXX_FLAGS})
try_compile(
   _has_cxx_class_template_argument_deduction
   "${CMAKE_BINARY_DIR}"
   "${CMAKE_SOURCE_DIR}/cmake/class-template-argument-deduction.cpp"
   CMAKE_FLAGS ${CMAKE_CXX_FLAGS})

include(CheckIncludeFileCXX)

check_include_file_cxx(
   any
   _has_cxx_std_any)

add_executable(
   ${proj_name}
   gl-fence-sync.cpp
   gl-fence-sync.h
   main.cpp
   multisample.h
   osg-gc-wrapper.cpp
   osg-gc-wrapper.h
   osg-view.cpp
   osg-view.h
   osg-view-mouse-event.cpp
   osg-view-mouse-event.h
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
   osgUtil
   osgText)

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

target_compile_definitions(
   ${proj_name}
   PRIVATE
   "_has_cxx_std_any=$<IF:$<BOOL:${_has_cxx_std_any}>,1,0>"
   "_has_cxx_structured_bindings=$<IF:$<BOOL:${_has_cxx_structured_bindings}>,1,0>"
   "_has_cxx_std_map_extract=$<IF:$<BOOL:${_has_cxx_std_map_extract}>,1,0>"
   "_has_cxx_std_shared_ptr_weak_type=$<IF:$<BOOL:${_has_cxx_std_shared_ptr_weak_type}>,1,0>"
   "_has_cxx_class_template_argument_deduction=$<IF:$<BOOL:${_has_cxx_class_template_argument_deduction}>,1,0>")
