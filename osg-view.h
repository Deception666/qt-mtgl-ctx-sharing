#ifndef _OSG_VIEW_H_
#define _OSG_VIEW_H_

#include <QtCore/QObject>
#include <QtCore/QPoint>

#include <osg/ref_ptr>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/GL.h>
#elif __linux__
#include <GL/gl.h>
#else
#error "Define for this platform!"
#endif

#if _has_cxx_std_any
#include <any>
#else
#include "stl-ext/any"
#endif

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class QEvent;

namespace osg
{
class FrameBufferObject;
class GraphicsContext;
class Texture;
class Texture2D;
class Texture2DMultisample;
}

namespace osgUtil
{
class SceneView;
}

namespace gl
{
class FenceSync;
}

enum class Multisample;

class OSGView :
   public QObject
{
   Q_OBJECT;

public:
   OSGView(
      const int32_t width,
      const int32_t height,
      const Multisample multisample,
      const QObject & parent,
      const std::string & model ) noexcept;
   ~OSGView( ) noexcept;

   void PreRender( ) noexcept;
   void Render( ) noexcept;
   void PostRender( ) noexcept;

signals:
   void Present(
      const std::shared_ptr<
         std::pair< GLuint, gl::FenceSync > >  & fence_sync );

protected:
   bool event(
      QEvent * const event ) final;

private slots:
   void OnResize(
      const int32_t width,
      const int32_t height ) noexcept;
   void OnPresentComplete(
      const std::shared_ptr<
         std::pair< GLuint, gl::FenceSync > > & fence_sync ) noexcept;
   void OnSetCameraLookAt(
      const std::array< double, 3 > & eye,
      const std::array< double, 3 > & center,
      const std::array< double, 3 > & up ) noexcept;

private:
   void SetupOSG(
      const std::string & model ) noexcept;
   void SetupFrameBuffer(
      const Multisample multisample ) noexcept;
   void SetupSignalsSlots( ) noexcept;
   void ReleaseSignalsSlots( ) noexcept;
   osg::ref_ptr< osg::Texture >
   SetupDepthBuffer(
      const Multisample multisample ) noexcept;
   osg::ref_ptr< osg::Texture2DMultisample >
   SetupStencilBuffer(
      const Multisample multisample ) noexcept;
   osg::ref_ptr< osg::Texture2DMultisample >
   SetupMultisampleBuffer(
      const Multisample multisample ) noexcept;

   void UpdateText(
      const GLuint color_buffer_texture_id ) const noexcept;
   void UpdateTextProjection( ) const noexcept;

   std::pair< bool, GLuint > SetupNextFrame( ) noexcept;
   std::pair< GLuint, osg::ref_ptr< osg::FrameBufferObject > >
   GetNextFrameBuffer( ) noexcept;

   uint32_t width_;
   uint32_t height_;

   osg::ref_ptr< osgUtil::SceneView > osg_scene_view_;

   osg::ref_ptr< osg::FrameBufferObject > multisample_frame_buffer_;

   std::map< GLuint, osg::ref_ptr< osg::FrameBufferObject > > active_frame_buffers_;
   std::map< GLuint, osg::ref_ptr< osg::FrameBufferObject > > inactive_frame_buffers_;

   std::vector<
      std::shared_ptr<
         std::pair< GLuint, gl::FenceSync > > > completed_frames_;

   QPoint previous_mouse_pos_;

   const QObject & parent_;
   const osg::ref_ptr< osg::GraphicsContext > graphics_context_;

};

#endif // _OSG_VIEW_H_
