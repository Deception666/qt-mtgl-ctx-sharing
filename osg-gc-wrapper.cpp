#include "osg-gc-wrapper.h"

#include <osg/State>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include <cassert>
#include <tuple>
#include <utility>

OSGGraphicsContextWrapper::OSGGraphicsContextWrapper(
   const std::any & gl_context )
{
#if _WIN32

#if _has_cxx_structured_bindings
   const auto & [window, device_context, wgl_context] =
#else
   const auto & gl_context_data =
#endif
      std::any_cast<
         std::tuple< HWND, HDC, HGLRC > >(
            gl_context);

#if !_has_cxx_structured_bindings
   const HWND window = std::get< 0 >(gl_context_data);
   const HDC device_context = std::get< 1 >(gl_context_data);
   const HGLRC wgl_context = std::get< 2 >(gl_context_data);
#endif
   setHWND(window);
   setHDC(device_context);
   setWGLContext(wgl_context);

#elif __linux__

#if _has_cxx_structured_bindings
   const auto & [display, glx_context] =
#else
   const auto & gl_context_data =
#endif
      std::any_cast<
         std::tuple< Display *, GLXContext > >(
            gl_context);

#if !_has_cxx_structured_bindings
   const Display * const display = std::get< 0 >(gl_context_data);
   const GLXContext glx_context = std::get< 1 >(gl_context_data);
#endif

   setDisplay(display);
   setContext(glx_context);

#else
#error "Define for this platform!"
#endif

   setState(
      new osg::State);

   getState()->setContextID(
      osg::GraphicsContext::createNewContextID());
}

bool OSGGraphicsContextWrapper::valid( ) const
{
#ifdef _WIN32
   return getWGLContext();
#elif __linux__
   return
      getDisplay() &&
      getContext();
#else
#error "Define for this platform!"
#endif
}

bool OSGGraphicsContextWrapper::realizeImplementation( )
{
   assert(false);

   return false;
}

bool OSGGraphicsContextWrapper::isRealizedImplementation( ) const
{
   assert(false);

   return true;
}

void OSGGraphicsContextWrapper::closeImplementation( )
{
   assert(false);
}

bool OSGGraphicsContextWrapper::makeCurrentImplementation( )
{
   assert(false);

   return false;
}

bool OSGGraphicsContextWrapper::makeContextCurrentImplementation(
   GraphicsContext * const readContext )
{
   assert(false);

   return false;
}

bool OSGGraphicsContextWrapper::releaseContextImplementation( )
{
#if _WIN32

   setHWND(nullptr);
   setHDC(nullptr);
   setWGLContext(nullptr);

#elif __linux__

   setDisplay(nullptr);
   setContext(nullptr);

#else
#error "Define for this platform!"
#endif

   return true;
}

void OSGGraphicsContextWrapper::bindPBufferToTextureImplementation(
   const GLenum buffer )
{
   assert(false);
}

void OSGGraphicsContextWrapper::swapBuffersImplementation( )
{
   assert(false);
}
