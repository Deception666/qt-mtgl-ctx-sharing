#include "qt-gl-view.h"
#include "color-buffer-data.h"
#include "render-thread.h"
#include "osg-view.h"

#if _WIN32
#include <QtPlatformHeaders/QWGLNativeContext>
#endif // _WIN32

#include <QtGui/QCloseEvent>
#include <QtGui/QOpenGLContext>

#include <QtCore/QVariant>

#include <tuple>
#include <utility>

void ReleaseOSGView(
   const OSGView * const osg_view ) noexcept
{
   if (osg_view)
   {
      render_thread::AddOperation(
         [ osg_view ] ( )
         {
            delete osg_view;
         }).wait();
   }
}

QtGLView::QtGLView(
   std::string model,
   QWidget * const parent ) noexcept :
QOpenGLWidget { parent },
current_frame_ { nullptr },
osg_view_ { nullptr },
model_ { std::move(model) }
{
}

void QtGLView::initializeGL( )
{
   QOpenGLWidget::initializeGL();

   doneCurrent();

   render_thread::AddOperation(
      [ this ] ( )
      {
         // note the context defined during the initialize gl
         // call is valid but the window is temporary...  this
         // means that the dc cannot be used to make current,
         // but that should not be an issue as the bridge between
         // the contexts are needed only.
         osg_view_.reset(
            new OSGView {
               width(),
               height(),
               *this,
#if _WIN32
               std::any {
                  std::make_tuple(
                     qvariant_cast< QWGLNativeContext >(
                        context()->nativeHandle()).window(),
                     GetDC(
                        qvariant_cast< QWGLNativeContext >(
                           context()->nativeHandle()).window()),
                     qvariant_cast< QWGLNativeContext >(
                        context()->nativeHandle()).context()) },
#endif // _WIN32
               model_ },
            &ReleaseOSGView);

         render_thread::RegisterOSGView(
            decltype(osg_view_)::weak_type { osg_view_ });
      }).wait();

   SetupSignalsSlots();

   makeCurrent();
}

void QtGLView::resizeGL(
   const int32_t width,
   const int32_t height )
{
   QOpenGLWidget::resizeGL(
      width,
      height);

   emit
      Resize(width, height);
}

void QtGLView::paintGL( )
{
   QOpenGLWidget::paintGL();

   if (current_frame_)
   {
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      glViewport(
         0, 0,
         width(), height());

      glEnable(GL_TEXTURE_2D);

      glBindTexture(
         GL_TEXTURE_2D,
         current_frame_->color_buffer_texture_id_);

      const float tex_coords[] {
         0.0f, 1.0f,
         0.0f, 0.0f,
         1.0f, 0.0f,
         0.0f, 1.0f,
         1.0f, 0.0f,
         1.0f, 1.0f
      };

      const float vertices[] {
         -1.0f, 1.0f, 0.0f,
         -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         -1.0f, 1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f, 1.0f, 0.0f
      };

      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);

      glVertexPointer(
         3,
         GL_FLOAT,
         sizeof(float) * 3,
         vertices);
      glTexCoordPointer(
         2,
         GL_FLOAT,
         sizeof(float) * 2,
         tex_coords);

      glDrawArrays(
         GL_TRIANGLES,
         0,
         6);

      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);

      glBindTexture(
         GL_TEXTURE_2D,
         0);

      glDisable(GL_TEXTURE_2D);
   }
}

void QtGLView::closeEvent(
   QCloseEvent * const event )
{
   ReleaseSignalsSlots();

   render_thread::UnregisterOSGView(
      decltype(osg_view_)::weak_type { osg_view_ });
}

void QtGLView::SetupSignalsSlots( ) noexcept
{
   if (osg_view_)
   {
      QObject::connect(
         osg_view_.get(),
         &OSGView::Present,
         this,
         &QtGLView::OnPresent);
   }
}

void QtGLView::ReleaseSignalsSlots( ) noexcept
{
   if (osg_view_)
   {
      QObject::disconnect(
         osg_view_.get(),
         &OSGView::Present,
         this,
         &QtGLView::OnPresent);
   }
}

QtGLView::Frame * QtGLView::GetFrame(
   const uint32_t id )
{
   Frame * frame { nullptr };

   auto frame_it =
      active_frames_.find(id);

   if (frame_it == active_frames_.cend())
   {
      uint32_t texture_id { 0 };

      glGenTextures(
         1,
         &texture_id);

      glBindTexture(
         GL_TEXTURE_2D,
         texture_id);

      glTexParameteri(
         GL_TEXTURE_2D,
         GL_TEXTURE_MIN_FILTER,
         GL_NEAREST);
      glTexParameteri(
         GL_TEXTURE_2D,
         GL_TEXTURE_MAG_FILTER,
         GL_NEAREST);
      glTexParameteri(
         GL_TEXTURE_2D,
         GL_TEXTURE_WRAP_S,
         GL_CLAMP_TO_EDGE);
      glTexParameteri(
         GL_TEXTURE_2D,
         GL_TEXTURE_WRAP_T,
         GL_CLAMP_TO_EDGE);

      glBindTexture(
         GL_TEXTURE_2D,
         0);

      frame_it =
         active_frames_.emplace(
            id,
            Frame { texture_id, { } }).first;
   }

   frame = &frame_it->second;

   return frame;
}

void QtGLView::OnPresent(
   const std::shared_ptr< ColorBufferData > & color_buffer ) noexcept
{
   makeCurrent();

   const auto frame =
      GetFrame(color_buffer->id_);

   frame->pbo_.Bind(
      gl::PixelBufferObject::Operation::UNPACK);

   const uint32_t data_size_in_bytes =
      color_buffer->data_.size() * sizeof(uint32_t);
   
   if (data_size_in_bytes > frame->pbo_.GetSize())
   {
      frame->pbo_.SetSize(
         data_size_in_bytes);
   }

   frame->pbo_.WriteData(
      0,
      data_size_in_bytes,
      color_buffer->data_.data());

   glBindTexture(
      GL_TEXTURE_2D,
      frame->color_buffer_texture_id_);

   glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA8,
      color_buffer->width_,
      color_buffer->height_,
      0,
      GL_BGRA,
      GL_UNSIGNED_BYTE,
      nullptr);

   glBindTexture(
      GL_TEXTURE_2D,
      0);

   frame->pbo_.Bind(
      gl::PixelBufferObject::Operation::NONE);

   doneCurrent();

   current_frame_ = frame;

   update();

   emit
      PresentComplete(
         color_buffer);
}
