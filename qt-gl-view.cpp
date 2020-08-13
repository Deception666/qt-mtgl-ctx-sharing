#include "qt-gl-view.h"
#include "gl-fence-sync.h"
#include "multisample.h"
#include "osg-view.h"
#include "osg-view-mouse-event.h"
#include "render-thread.h"

#include <QtGui/QCloseEvent>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShader>

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>

#include <Qt>

void ReleaseOSGView(
   const OSGView * const osg_view ) noexcept
{
   if (osg_view)
   {
      render_thread::AddOperation(
         [ osg_view ] ( )
         {
            QCoreApplication::sendPostedEvents(
               const_cast< OSGView * >(osg_view));

            delete osg_view;
         }).wait();
   }
}

QtGLView::QtGLView(
   std::string model,
   QWidget * const parent ) noexcept :
QOpenGLWidget { parent },
render_scene_pgm_ { this },
scene_data_vao_ { this },
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

   if (osg_view_)
   {
      render_scene_pgm_.create();
      
      render_scene_pgm_.addShaderFromSourceCode(
         QOpenGLShader::Vertex,
         "#version 330\n"
         "#extension GL_ARB_shading_language_420pack : require\n"
         "const vec4 vertices[6] ="
         "{"
         "  vec4(-1.0f, 1.0f, 0.0f, 1.0f),"
         "  vec4(-1.0f, -1.0f, 0.0f, 1.0f),"
         "  vec4(1.0f, -1.0f, 0.0f, 1.0f),"
         "  vec4(-1.0f, 1.0f, 0.0f, 1.0f),"
         "  vec4(1.0f, -1.0f, 0.0f, 1.0f),"
         "  vec4(1.0f, 1.0f, 0.0f, 1.0f)"
         "};"
         ""
         "const vec2 texture_coords[6] ="
         "{"
         "  vec2(0.0f, 1.0f),"
         "  vec2(0.0f, 0.0f),"
         "  vec2(1.0f, 0.0f),"
         "  vec2(0.0f, 1.0f),"
         "  vec2(1.0f, 0.0f),"
         "  vec2(1.0f, 1.0f)"
         "};"
         ""
         "smooth out vec2 texture_coord;"
         ""
         "void main( void )"
         "{"
         "  gl_Position = vertices[gl_VertexID];"
         "  texture_coord = texture_coords[gl_VertexID];"
         "}");
      
      render_scene_pgm_.addShaderFromSourceCode(
         QOpenGLShader::Fragment,
         "#version 330\n"
         ""
         "uniform sampler2D frame_sampler_2d;"
         ""
         "smooth in vec2 texture_coord;"
         ""
         "layout( location = 0 ) out vec4 frag_color_0;"
         ""
         "void main( void )"
         "{"
         "  frag_color_0 = texture(frame_sampler_2d, texture_coord);"
         "}");

      render_scene_pgm_.link();

      render_scene_pgm_.setUniformValue(
         "frame_sampler_2d",
         0);

      scene_data_vao_.create();

      setMouseTracking(true);
   }
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
      render_scene_pgm_.bind();
      scene_data_vao_.bind();

      const auto gl_functions =
         context()->functions();
      gl_functions->glActiveTexture(
         GL_TEXTURE0);

      glBindTexture(
         GL_TEXTURE_2D,
         current_color_buffer_->first);

      glDrawArrays(
         GL_TRIANGLES,
         0,
         6);

      scene_data_vao_.release();
      render_scene_pgm_.release();
   }
}

void QtGLView::closeEvent(
   QCloseEvent * const event )
{
   ReleaseSignalsSlots();

   QCoreApplication::sendPostedEvents(
      this,
      QEvent::Type::MetaCall);

   render_thread::UnregisterOSGView(
#if _has_cxx_std_shared_ptr_weak_type
      decltype(osg_view_)::weak_type { osg_view_ });
#else
      std::weak_ptr< OSGView > { osg_view_ });
#endif

   makeCurrent();
   scene_data_vao_.destroy();
   doneCurrent();

   if (current_color_buffer_)
   {
      emit PresentComplete(
         current_color_buffer_);

      current_color_buffer_ = nullptr;
   }

   for (const auto & color_buffer : waiting_color_buffers_)
   {
      emit PresentComplete(
         color_buffer);
   }

   waiting_color_buffers_.clear();
}

void QtGLView::mouseMoveEvent(
   QMouseEvent * const event )
{
   if (osg_view_)
   {
      QCoreApplication::postEvent(
         osg_view_.get(),
         new MouseEvent {
            static_cast< QEvent::Type >(MOUSE_MOVE_EVENT),
            *event },
         Qt::NormalEventPriority);
   }
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
