#ifndef _QT_GL_VIEW_H_
#define _QT_GL_VIEW_H_

#include "gl-fence-sync.h"

#include <QtWidgets/QOpenGLWidget>
#include <QtWidgets/QWidget>

#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLVertexArrayObject>

#include <QtCore/QObject>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GL/GL.h>

#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <utility>

class OSGView;
class QCloseEvent;
class QMouseEvent;

class QtGLView final :
   public QOpenGLWidget
{
   Q_OBJECT;

public:
   QtGLView(
      std::string model,
      QWidget * const parent ) noexcept;

signals:
   void Resize(
      const int32_t width,
      const int32_t height );
   void PresentComplete(
      const std::shared_ptr<
         std::pair< GLuint, gl::FenceSync > > & fence_sync );
   void SetCameraLookAt(
      const std::array< double, 3 > & eye,
      const std::array< double, 3 > & center,
      const std::array< double, 3 > & up );

protected:
   void initializeGL( ) final;
   void resizeGL(
      const int32_t width,
      const int32_t height ) final;
   void paintGL( ) final;

   void closeEvent(
      QCloseEvent * const event ) final;
   void mouseMoveEvent(
      QMouseEvent * const event ) final;

private slots:
   void OnPresent(
      const std::shared_ptr<
         std::pair< GLuint, gl::FenceSync > > & fence_sync ) noexcept;

private:
   void SetupSignalsSlots( ) noexcept;
   void ReleaseSignalsSlots( ) noexcept;

   std::shared_ptr< std::pair< GLuint, gl::FenceSync > >
      current_color_buffer_;
   std::list<
      std::shared_ptr< std::pair< GLuint, gl::FenceSync > > >
      waiting_color_buffers_;

   QOpenGLShaderProgram render_scene_pgm_;
   QOpenGLVertexArrayObject scene_data_vao_;

   std::shared_ptr< OSGView > osg_view_;

   const std::string model_;

};

#endif // _QT_GL_VIEW_H_
