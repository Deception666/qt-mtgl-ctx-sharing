#include "qt-gl-view.h"
#include "gl-fence-sync.h"
#include "multisample.h"
#include "osg-view.h"
#include "render-thread.h"

#include <QtGui/QCloseEvent>
#include <QtGui/QOpenGLContext>

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
               Multisample::SIXTEEN,
               *this,
               model_ },
            &ReleaseOSGView);

         render_thread::RegisterOSGView(
#if _has_cxx_std_shared_ptr_weak_type
            decltype(osg_view_)::weak_type { osg_view_ });
#else
            std::weak_ptr< OSGView > { osg_view_ });
#endif
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

   {
      auto color_buffer =
         waiting_color_buffers_.cbegin();

      while (waiting_color_buffers_.cend() != color_buffer)
      {
         if (!(*color_buffer)->second.IsSignaled())
         {
            update();

            break;
         }
         else
         {
            if (current_color_buffer_)
            {
               emit PresentComplete(
                  current_color_buffer_);
            }

            current_color_buffer_ =
               *color_buffer;

            color_buffer =
               waiting_color_buffers_.erase(
                  color_buffer);
         }
      }
   }

   if (current_color_buffer_)
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
         current_color_buffer_->first);

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
#if _has_cxx_std_shared_ptr_weak_type
      decltype(osg_view_)::weak_type { osg_view_ });
#else
      std::weak_ptr< OSGView > { osg_view_ });
#endif
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

void QtGLView::OnPresent(
   const std::shared_ptr<
      std::pair< GLuint, gl::FenceSync > > & fence_sync ) noexcept
{
   if (fence_sync &&
       fence_sync->second.Valid())
   {
      waiting_color_buffers_.emplace_back(
         fence_sync);

      update();
   }
}
