#ifndef _OSG_GC_WRAPPER_H_
#define _OSG_GC_WRAPPER_H_

#include <osg/GraphicsContext>

#if _WIN32
#include <osgViewer/api/Win32/GraphicsHandleWin32>
using OSGViewerGraphicsHandle = osgViewer::GraphicsHandleWin32;
#elif __linux__
#include <osgViewer/api/X11/GraphicsHandleX11>
using OSGViewerGraphicsHandle = osgViewer::GraphicsHandleX11;
#else
#error "Define for this platform!"
#endif

#if _has_cxx_std_any
#include <any>
#else
#include "stl-ext/any"
#endif

class OSGGraphicsContextWrapper final :
   public osg::GraphicsContext,
   public OSGViewerGraphicsHandle
{
public:
   OSGGraphicsContextWrapper(
      const std::any & gl_context );

   bool valid( ) const override;
   bool realizeImplementation( ) override;
   bool isRealizedImplementation( ) const override;
   void closeImplementation( ) override;
   bool makeCurrentImplementation( ) override;
   bool makeContextCurrentImplementation(
      GraphicsContext * const readContext ) override;
   bool releaseContextImplementation( ) override;
   void bindPBufferToTextureImplementation(
      const GLenum buffer ) override;
   void swapBuffersImplementation( ) override;

private:

};

#endif // _OSG_GC_WRAPPER_H_
