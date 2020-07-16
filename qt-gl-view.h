#ifndef _QT_GL_VIEW_H_
#define _QT_GL_VIEW_H_

#include "gl-pixel-buffer-object.h"

#include <QtWidgets/QOpenGLWidget>
#include <QtWidgets/QWidget>

#include <QtCore/QObject>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GL/GL.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>

class OSGView;
class QCloseEvent;

struct ColorBufferData;

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
      const std::shared_ptr< ColorBufferData > & color_buffer );
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

private slots:
   void OnPresent(
      const std::shared_ptr< ColorBufferData > & color_buffer ) noexcept;

private:
   void SetupSignalsSlots( ) noexcept;
   void ReleaseSignalsSlots( ) noexcept;

   struct Frame
   {
      GLuint color_buffer_texture_id_;
      gl::PixelBufferObject pbo_;
   };

   Frame * GetFrame(
      const uint32_t id );

   const Frame * current_frame_;
   std::map< uint32_t, Frame > active_frames_;

   std::shared_ptr< OSGView > osg_view_;

   const std::string model_;

};

#endif // _QT_GL_VIEW_H_
