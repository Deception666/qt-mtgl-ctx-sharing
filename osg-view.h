#ifndef _OSG_VIEW_H_
#define _OSG_VIEW_H_

#include "color-buffer-data.h"

#include <QtCore/QObject>

#include <osg/ref_ptr>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GL/GL.h>

#include <any>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace osg
{
class FrameBufferObject;
class GraphicsContext;
class Texture2D;
}

namespace osgUtil
{
class SceneView;
}

class OSGGraphicsContextWrapper;
struct ColorBufferData;

class OSGView :
   public QObject
{
   Q_OBJECT;

public:
   OSGView(
      const int32_t width,
      const int32_t height,
      const QObject & parent,
      const std::any parent_gl_context,
      const std::string & model ) noexcept;
   ~OSGView( ) noexcept;

   void PreRender( ) noexcept;
   void Render( ) noexcept;
   void PostRender( ) noexcept;

signals:
   void Present(
      const std::shared_ptr< ColorBufferData > & color_buffer );

protected:

private slots:
   void OnResize(
      const int32_t width,
      const int32_t height ) noexcept;
   void OnPresentComplete(
      const std::shared_ptr< ColorBufferData > & color_buffer ) noexcept;
   void OnSetCameraLookAt(
      const std::array< double, 3 > & eye,
      const std::array< double, 3 > & center,
      const std::array< double, 3 > & up ) noexcept;

private:
   void SetupOSG(
      const std::string & model ) noexcept;
   void SetupFrameBuffer( ) noexcept;
   void SetupSignalsSlots( ) noexcept;
   void ReleaseSignalsSlots( ) noexcept;
   osg::ref_ptr< osg::Texture2D >
   SetupDepthBuffer( ) noexcept;

   std::pair< bool, GLuint > SetupNextFrame( ) noexcept;
   std::pair< GLuint, osg::ref_ptr< osg::FrameBufferObject > >
   GetNextFrameBuffer( ) noexcept;

   uint32_t width_;
   uint32_t height_;

   osg::ref_ptr< osgUtil::SceneView > osg_scene_view_;

   std::map< GLuint, osg::ref_ptr< osg::FrameBufferObject > > active_frame_buffers_;
   std::map< GLuint, osg::ref_ptr< osg::FrameBufferObject > > inactive_frame_buffers_;

   const QObject & parent_;
   const osg::ref_ptr< OSGGraphicsContextWrapper > parent_gc_wrapper_;
   const osg::ref_ptr< osg::GraphicsContext > graphics_context_;

};

#endif // _OSG_VIEW_H_
