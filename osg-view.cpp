#include "osg-view.h"
#include "gl-fence-sync.h"
#include "multisample.h"
#include "osg-gc-wrapper.h"

#if _WIN32
#include <osgViewer/api/Win32/GraphicsHandleWin32>
#include <osgViewer/api/Win32/GraphicsWindowWin32>
#endif // _WIN32

#include <osgUtil/RenderStage>
#include <osgUtil/SceneView>
#include <osgUtil/UpdateVisitor>

#include <osgText/Text>

#include <osgDB/ReadFile>

#include <osg/Camera>
#include <osg/DisplaySettings>
#include <osg/FrameStamp>
#include <osg/FrameBufferObject>
#include <osg/GLExtensions>
#include <osg/GraphicsContext>
#include <osg/Matrix>
#include <osg/MatrixTransform>
#include <osg/ref_ptr>
#include <osg/Texture>
#include <osg/Texture2D>
#include <osg/Texture2DMultisample>
#include <osg/Vec3>

#include <QMetaType>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include <cassert>
#include <chrono>
#include <iostream>
#include <type_traits>

#define USE_GL_FLUSH 1
#define USE_GL_FINISH 0
#define SHARE_WITH_PARENT_GC_CONTEXT 0
#define USE_SINGLE_DEPTH_STENCIL_MULTISAMPLE_ATTACHMENT 1

static const auto qt_meta_type_int32_t =
   qRegisterMetaType< int32_t >("int32_t");
static const auto qt_meta_type_GLuint =
   qRegisterMetaType< GLuint >("GLuint");
static const auto qt_meta_type_std_array_doulbe_3 =
   qRegisterMetaType< std::array< double, 3 > >(
      "std::array< double, 3 >");

#if _WIN32
static std::unique_ptr<
   std::remove_pointer_t< HMODULE >,
   decltype(&FreeLibrary) >
   graphics_subsystem_module_ {
      nullptr,
      &FreeLibrary };
#else // _WIN32
static std::unique_ptr< uintptr_t >
   graphics_subsystem_module_ { nullptr };
#endif // _WIN32

void LoadGraphicsSubsystem( ) noexcept
{
#if _WIN32
   graphics_subsystem_module_.reset(
      LoadLibrary(
#if !NDEBUG
         "osgViewerd.dll")
#else // !NDEBUG
         "osgViewer.dll")
#endif // !NDEBUG
      );
#endif // _WIN32
}

void HideContextWindow(
   const osg::ref_ptr< osg::GraphicsContext > & context ) noexcept
{
   if (context)
   {
#if _WIN32
      const auto window =
         dynamic_cast< osgViewer::GraphicsWindowWin32 * >(
            context.get());

      if (window && window->getHWND())
      {
         ShowWindow(
            window->getHWND(),
            SW_HIDE);
      }
#endif // _WIN32
   }
}

osg::ref_ptr< osg::GraphicsContext >
CreateGraphicsContext(
   const int32_t width,
   const int32_t height,
   const char * const window_name,
   osg::ref_ptr< osg::GraphicsContext > share_context ) noexcept
{
   osg::ref_ptr< osg::GraphicsContext::Traits > gc_traits {
      new osg::GraphicsContext::Traits { nullptr } };

   gc_traits->x = 0;
   gc_traits->y = 0;
   gc_traits->width = width;
   gc_traits->height = height;

   gc_traits->windowingSystemPreference = "Win32";

   gc_traits->windowName = window_name;
   gc_traits->windowDecoration = false;
   gc_traits->supportsResize = true;

   gc_traits->red = 8;
   gc_traits->blue = 8;
   gc_traits->green = 8;
   gc_traits->alpha = 8;
   gc_traits->depth = 24;
   gc_traits->stencil = 8;

   gc_traits->sampleBuffers = 0;
   gc_traits->samples = 0;

   gc_traits->pbuffer = false;
   gc_traits->quadBufferStereo = false;
   gc_traits->doubleBuffer = true;

   gc_traits->target = 0;
   gc_traits->format = 0;
   gc_traits->level = 0;
   gc_traits->face = 0;
   gc_traits->mipMapGeneration = false;

   gc_traits->vsync = false;

   gc_traits->swapGroupEnabled = false;
   gc_traits->swapGroup = 0;
   gc_traits->swapBarrier = 0;

   gc_traits->useMultiThreadedOpenGLEngine = false;

   gc_traits->useCursor = false;

   gc_traits->glContextVersion = "4.0";
   gc_traits->glContextFlags = 0;
   gc_traits->glContextProfileMask = 0;

   gc_traits->sharedContext = share_context;
         
   gc_traits->inheritedWindowData = nullptr;

   gc_traits->setInheritedWindowPixelFormat = false;

   gc_traits->overrideRedirect = false;

   gc_traits->swapMethod = osg::DisplaySettings::SWAP_DEFAULT;

   const osg::ref_ptr< osg::GraphicsContext > graphics_context =
      osg::GraphicsContext::createGraphicsContext(
         gc_traits.get());

   if (graphics_context)
   {
      graphics_context->realize();

      HideContextWindow(
         graphics_context);
   }

   return graphics_context;
}

static osg::ref_ptr< osg::GraphicsContext >
   hidden_graphics_context_;

void InitHiddenGLContext(
   const std::any & hidden_context ) noexcept
{
   if (!hidden_graphics_context_)
   {
      LoadGraphicsSubsystem();

      if (!hidden_context.has_value())
      {
         hidden_graphics_context_ =
            CreateGraphicsContext(
               1, 1,
               "hidden graphics context",
               nullptr);
      }
      else
      {
#if _WIN32
         hidden_graphics_context_ =
            new OSGGraphicsContextWrapper {
               hidden_context };
#else
#error "Define for this platform!"
#endif
      }
   }
}

void ReleaseHiddenGLContext( ) noexcept
{
   if (hidden_graphics_context_)
   {
      hidden_graphics_context_ = nullptr;

      graphics_subsystem_module_ = nullptr;
   }
}

OSGView::OSGView(
   const int32_t width,
   const int32_t height,
   const Multisample multisample,
   const QObject & parent,
   const std::any parent_gl_context,
   const std::string & model ) noexcept :
width_ { static_cast< uint32_t >(width) },
height_ { static_cast< uint32_t >(height) },
osg_scene_view_ { new osgUtil::SceneView { nullptr } },
QObject { nullptr },
parent_ { parent },
parent_gc_wrapper_ {
   new OSGGraphicsContextWrapper {
      parent_gl_context } },
graphics_context_ {
   CreateGraphicsContext(
      1, 1,
      "osg-view-context",
      hidden_graphics_context_) }
{
   assert(graphics_context_.get());

#if SHARE_WITH_PARENT_GC_CONTEXT
#if _WIN32
   const auto shared =
      wglShareLists(
         parent_gc_wrapper_->getWGLContext(),
         dynamic_cast< osgViewer::GraphicsHandleWin32 * >(
            graphics_context_.get())->getWGLContext());

   assert(shared);
#else
#error "Define for this platform!"
#endif // _WIN32
#endif // SHARE_WITH_PARENT_GC_CONTEXT

   SetupOSG(model);
   SetupFrameBuffer(multisample);
   SetupSignalsSlots();
}

OSGView::~OSGView( ) noexcept
{
   ReleaseSignalsSlots();
}

void OSGView::PreRender( ) noexcept
{
   const auto mtransform =
      static_cast< osg::MatrixTransform * >(
         osg_scene_view_->getSceneData());

   const auto matrix =
      mtransform->getMatrix() *
      osg::Matrix::rotate(0.0175, 0.0, 0.0, 1.0);

   mtransform->setMatrix(
      matrix);
}

void OSGView::Render( ) noexcept
{
   if (osg_scene_view_)
   {
      graphics_context_->makeCurrent();

#if _has_cxx_structured_bindings
      const auto [next_frame_setup, color_buffer_id] =
         SetupNextFrame();
#else
      const auto next_frame =
         SetupNextFrame();

      const auto next_frame_setup = next_frame.first;
      const auto color_buffer_id = next_frame.second;
#endif

      if (next_frame_setup)
      {
         const auto frame_stamp =
            osg_scene_view_->getFrameStamp();

         frame_stamp->setFrameNumber(
            frame_stamp->getFrameNumber() + 1);
         frame_stamp->setSimulationTime(
            std::chrono::duration_cast< std::chrono::milliseconds >(
               std::chrono::steady_clock::now().time_since_epoch()).count() /
            1000.0);

         UpdateText(
            color_buffer_id);

         osg_scene_view_->update();
         osg_scene_view_->cull();
         osg_scene_view_->draw();

         gl::FenceSync fence_sync;

#if USE_GL_FLUSH
         glFlush();
#endif

#if USE_GL_FINISH
         glFinish();
#endif

         if (fence_sync.Valid() &&
             !fence_sync.IsSignaled())
         {
            active_fence_syncs_.emplace_back(
               color_buffer_id,
               std::make_unique< gl::FenceSync >(
                  std::move(fence_sync)));
         }
         else
         {
            emit Present(color_buffer_id);
         }
      }

      graphics_context_->releaseContext();
   }
}

void OSGView::PostRender( ) noexcept
{
}

void OSGView::OnPresentComplete(
   const GLuint color_buffer_texture_id ) noexcept
{
#if _has_cxx_std_map_extract
   inactive_frame_buffers_.insert(
      active_frame_buffers_.extract(
         color_buffer_texture_id));
#else
   const auto active_frame_buffer =
      active_frame_buffers_.find(
         color_buffer_texture_id);

   inactive_frame_buffers_.insert({
      active_frame_buffer->first,
      active_frame_buffer->second});

   active_frame_buffers_.erase(
      active_frame_buffer);
#endif
}

void OSGView::OnSetCameraLookAt(
   const std::array< double, 3 > & eye,
   const std::array< double, 3 > & center,
   const std::array< double, 3 > & up ) noexcept
{
   osg_scene_view_->getCamera()->setViewMatrix(
      osg::Matrix::lookAt(
         osg::Vec3d { eye[0], eye[1], eye[2] },
         osg::Vec3d { center[0], center[1], center[2] },
         osg::Vec3d { up[0], up[1], up[2] }));
}

void OSGView::SetupOSG(
   const std::string & model ) noexcept
{
   graphics_context_->getState()->get< osg::GLExtensions >(
      )->isTextureStorageEnabled = false;

   osg_scene_view_->setDefaults();

   osg_scene_view_->setState(
      graphics_context_->getState());

   osg_scene_view_->setUpdateVisitor(
      new osgUtil::UpdateVisitor);

   osg_scene_view_->setAutomaticFlush(true);

   osg_scene_view_->setRenderStage(
      new osgUtil::RenderStage);

   const auto model_node =
      osgDB::readRefNodeFile(
         model);

   const auto mtransform =
      new osg::MatrixTransform;

   mtransform->addChild(
      model_node);

   mtransform->addChild(
      new osgText::Text);

   osg_scene_view_->setSceneData(
      mtransform);

   osg_scene_view_->getCamera()->setProjectionMatrix(
      osg::Matrix::perspective(
         45.0,
         static_cast< double >(width_) /
         static_cast< double >(height_),
         1.0, 200.0));

   osg_scene_view_->getViewport()->setViewport(
      0.0, 0.0,
      width_, height_);
}

class FrameBufferSubloadCallback :
   public osg::Texture2D::SubloadCallback
{
public:
   bool IsPerformingResize( ) const
   {
      return performing_resize_;
   }

   void SetPerformingResize(
      const bool resizing )
   {
      performing_resize_ = resizing;
   }

   bool textureObjectValid(
      const osg::Texture2D & texture,
      osg::State & state ) const override
   {
      bool valid { true };

      if (!performing_resize_)
         valid =
            osg::Texture2D::SubloadCallback::textureObjectValid(
               texture,
               state);

      return valid;
   }
   
   void load(
      const osg::Texture2D & texture,
      osg::State & state ) const override
   {
      const auto texture_object =
         texture.getTextureObject(
            state.getContextID());

      glBindTexture(
         GL_TEXTURE_2D,
         texture_object->id());

      glTexImage2D(
         GL_TEXTURE_2D,
         0,
         texture.getInternalFormat(),
         texture.getTextureWidth(),
         texture.getTextureHeight(),
         texture.getBorderWidth(),
         texture.getSourceFormat(),
         texture.getSourceType(),
         nullptr);
   }

   void subload(
      const osg::Texture2D & texture,
      osg::State & state) const override
   {
      load(
         texture,
         state);
   }

private:
   bool performing_resize_ { false };

};

void OSGView::SetupFrameBuffer(
   const Multisample multisample ) noexcept
{
   graphics_context_->makeCurrent();

   const auto depth_buffer =
      SetupDepthBuffer(multisample);

   const auto multisample_buffer =
      SetupMultisampleBuffer(multisample);

   if (multisample_buffer)
   {
#if !USE_SINGLE_DEPTH_STENCIL_MULTISAMPLE_ATTACHMENT
      const auto stencil_buffer =
         SetupStencilBuffer(multisample);
#endif

      multisample_frame_buffer_ =
         new osg::FrameBufferObject;

      multisample_frame_buffer_->setAttachment(
         osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0,
         osg::FrameBufferAttachment { multisample_buffer });

#if USE_SINGLE_DEPTH_STENCIL_MULTISAMPLE_ATTACHMENT
      multisample_frame_buffer_->setAttachment(
         osg::FrameBufferObject::BufferComponent::PACKED_DEPTH_STENCIL_BUFFER,
         osg::FrameBufferAttachment {
            static_cast< osg::Texture2DMultisample * >(
               depth_buffer.get()) });
#else
      multisample_frame_buffer_->setAttachment(
         osg::FrameBufferObject::BufferComponent::DEPTH_BUFFER,
         osg::FrameBufferAttachment {
            static_cast< osg::Texture2DMultisample * >(
               depth_buffer.get()) });
      multisample_frame_buffer_->setAttachment(
         osg::FrameBufferObject::BufferComponent::STENCIL_BUFFER,
         osg::FrameBufferAttachment {
            stencil_buffer });
#endif

      multisample_frame_buffer_->apply(
         *graphics_context_->getState());

      osg_scene_view_->getRenderStage()->setFrameBufferObject(
         multisample_frame_buffer_);
   }

   for (size_t i { 0 }; i < 3; ++i)
   {
      osg::ref_ptr< osg::Texture2D > color_buffer {
         new osg::Texture2D };

      color_buffer->setTextureSize(width_, height_);
      color_buffer->setInternalFormat(GL_RGBA8);
      color_buffer->setSourceFormat(GL_RGBA);
      color_buffer->setSourceType(GL_UNSIGNED_BYTE);
      color_buffer->setWrap(
         osg::Texture::WrapParameter::WRAP_S,
         osg::Texture::WrapMode::CLAMP_TO_EDGE);
      color_buffer->setWrap(
         osg::Texture::WrapParameter::WRAP_T,
         osg::Texture::WrapMode::CLAMP_TO_EDGE);
      color_buffer->setFilter(
         osg::Texture::FilterParameter::MIN_FILTER,
         osg::Texture::FilterMode::NEAREST);
      color_buffer->setFilter(
         osg::Texture::FilterParameter::MAG_FILTER,
         osg::Texture::FilterMode::NEAREST);
      color_buffer->setResizeNonPowerOfTwoHint(false);

      color_buffer->setSubloadCallback(
         new FrameBufferSubloadCallback);

      osg::ref_ptr< osg::FrameBufferObject > frame_buffer {
         new osg::FrameBufferObject };

      frame_buffer->setAttachment(
         osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0,
         osg::FrameBufferAttachment { color_buffer });

      if (!multisample_buffer)
      {
         frame_buffer->setAttachment(
            osg::FrameBufferObject::BufferComponent::PACKED_DEPTH_STENCIL_BUFFER,
            osg::FrameBufferAttachment {
               static_cast< osg::Texture2D * >(
                  depth_buffer.get()) });
      }

      frame_buffer->apply(
         *graphics_context_->getState());

      const auto & color0_attachment =
         frame_buffer->getAttachment(
            osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0);
      const auto color_buffer_texture_object =
         color0_attachment.getTexture()->getTextureObject(
            graphics_context_->getState()->getContextID());
      
      inactive_frame_buffers_.emplace(
         color_buffer_texture_object->id(),
         frame_buffer);

      std::cout
         << "Color ID = "
         << color_buffer_texture_object->id()
         << std::endl;
   }

   graphics_context_->releaseContext();
}

void OSGView::SetupSignalsSlots( ) noexcept
{
   QObject::connect(
      &parent_,
      SIGNAL(Resize(const int32_t, const int32_t)),
      this,
      SLOT(OnResize(const int32_t, const int32_t)));
   QObject::connect(
      &parent_,
      SIGNAL(PresentComplete(const GLuint)),
      this,
      SLOT(OnPresentComplete(const GLuint)));
   QObject::connect(
      &parent_,
      SIGNAL(SetCameraLookAt(
         const std::array< double, 3 > &,
         const std::array< double, 3 > &,
         const std::array< double, 3 > &)),
      this,
      SLOT(OnSetCameraLookAt(
         const std::array< double, 3 > &,
         const std::array< double, 3 > &,
         const std::array< double, 3 > &)));
}

void OSGView::ReleaseSignalsSlots( ) noexcept
{
   QObject::disconnect(
      &parent_,
      SIGNAL(Resize(const int32_t, const int32_t)),
      this,
      SLOT(OnResize(const int32_t, const int32_t)));
   QObject::disconnect(
      &parent_,
      SIGNAL(PresentComplete(const GLuint)),
      this,
      SLOT(OnPresentComplete(const GLuint)));
   QObject::disconnect(
      &parent_,
      SIGNAL(SetCameraLookAt(
         const std::array< double, 3 > &,
         const std::array< double, 3 > &,
         const std::array< double, 3 > &)),
      this,
      SLOT(OnSetCameraLookAt(
         const std::array< double, 3 > &,
         const std::array< double, 3 > &,
         const std::array< double, 3 > &)));
}

#define GL_DEPTH_STENCIL 0x84F9
#define GL_DEPTH32F_STENCIL8 0x8CAD
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD

osg::ref_ptr< osg::Texture >
OSGView::SetupDepthBuffer(
   const Multisample multisample ) noexcept
{
   assert(
      graphics_context_->isCurrent());

   osg::ref_ptr< osg::Texture > depth_buffer {
      nullptr };

   if (multisample == Multisample::NONE)
   {
      const auto buffer =
         new osg::Texture2D;

      buffer->setTextureSize(width_, height_);
#if USE_SINGLE_DEPTH_STENCIL_MULTISAMPLE_ATTACHMENT
      buffer->setInternalFormat(GL_DEPTH32F_STENCIL8);
      buffer->setSourceFormat(GL_DEPTH_STENCIL);
      buffer->setSourceType(GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
#else
      buffer->setInternalFormat(GL_DEPTH_COMPONENT32F);
      buffer->setSourceFormat(GL_DEPTH_COMPONENT);
      buffer->setSourceType(GL_FLOAT);
#endif

      buffer->setSubloadCallback(
         new FrameBufferSubloadCallback);

      depth_buffer = buffer;
   }
   else
   {
      const auto buffer =
         new osg::Texture2DMultisample {
            static_cast< GLsizei >(multisample),
            GL_FALSE };

      buffer->setTextureSize(width_, height_);
#if USE_SINGLE_DEPTH_STENCIL_MULTISAMPLE_ATTACHMENT
      buffer->setInternalFormat(GL_DEPTH32F_STENCIL8);
      buffer->setSourceFormat(GL_DEPTH_STENCIL);
      buffer->setSourceType(GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
#else
      buffer->setInternalFormat(GL_DEPTH_COMPONENT32F);
      buffer->setSourceFormat(GL_DEPTH_COMPONENT);
      buffer->setSourceType(GL_FLOAT);
#endif

      depth_buffer = buffer;
   }

   depth_buffer->setWrap(
      osg::Texture::WrapParameter::WRAP_S,
      osg::Texture::WrapMode::CLAMP_TO_EDGE);
   depth_buffer->setWrap(
      osg::Texture::WrapParameter::WRAP_T,
      osg::Texture::WrapMode::CLAMP_TO_EDGE);
   depth_buffer->setFilter(
      osg::Texture::FilterParameter::MIN_FILTER,
      osg::Texture::FilterMode::NEAREST);
   depth_buffer->setFilter(
      osg::Texture::FilterParameter::MAG_FILTER,
      osg::Texture::FilterMode::NEAREST);
   depth_buffer->setResizeNonPowerOfTwoHint(false);

   depth_buffer->apply(
      *graphics_context_->getState());

   std::cout
      << "Depth ID = "
      << depth_buffer->getTextureObject(
            graphics_context_->getState()->getContextID())->id()
      << std::endl;

   return depth_buffer;
}

osg::ref_ptr< osg::Texture2DMultisample >
OSGView::SetupStencilBuffer(
   const Multisample multisample ) noexcept
{
   assert(
      graphics_context_->isCurrent());

   osg::ref_ptr< osg::Texture2DMultisample > stencil_buffer {
      nullptr };

   if (multisample != Multisample::NONE)
   {
      stencil_buffer =
         new osg::Texture2DMultisample {
            static_cast< GLsizei >(multisample),
            GL_FALSE };

      stencil_buffer->setTextureSize(width_, height_);
      stencil_buffer->setInternalFormat(GL_STENCIL_INDEX8_EXT);
      stencil_buffer->setSourceFormat(GL_STENCIL_INDEX);
      stencil_buffer->setSourceType(GL_UNSIGNED_BYTE);
      stencil_buffer->setWrap(
         osg::Texture::WrapParameter::WRAP_S,
         osg::Texture::WrapMode::CLAMP_TO_EDGE);
      stencil_buffer->setWrap(
         osg::Texture::WrapParameter::WRAP_T,
         osg::Texture::WrapMode::CLAMP_TO_EDGE);
      stencil_buffer->setFilter(
         osg::Texture::FilterParameter::MIN_FILTER,
         osg::Texture::FilterMode::NEAREST);
      stencil_buffer->setFilter(
         osg::Texture::FilterParameter::MAG_FILTER,
         osg::Texture::FilterMode::NEAREST);
      stencil_buffer->setResizeNonPowerOfTwoHint(false);

      stencil_buffer->apply(
         *graphics_context_->getState());
      
      std::cout
         << "Stencil ID = "
         << stencil_buffer->getTextureObject(
               graphics_context_->getState()->getContextID())->id()
         << std::endl;
   }

   return stencil_buffer;
}

osg::ref_ptr< osg::Texture2DMultisample >
OSGView::SetupMultisampleBuffer(
   const Multisample multisample ) noexcept
{
   assert(
      graphics_context_->isCurrent());

   osg::ref_ptr< osg::Texture2DMultisample > multisample_buffer {
      nullptr };

   if (multisample != Multisample::NONE)
   {
      multisample_buffer =
         new osg::Texture2DMultisample {
            static_cast< GLsizei >(multisample),
            GL_FALSE };

      multisample_buffer->setTextureSize(width_, height_);
      multisample_buffer->setInternalFormat(GL_RGBA8);
      multisample_buffer->setSourceFormat(GL_RGBA);
      multisample_buffer->setSourceType(GL_UNSIGNED_BYTE);
      multisample_buffer->setWrap(
         osg::Texture::WrapParameter::WRAP_S,
         osg::Texture::WrapMode::CLAMP_TO_EDGE);
      multisample_buffer->setWrap(
         osg::Texture::WrapParameter::WRAP_T,
         osg::Texture::WrapMode::CLAMP_TO_EDGE);
      multisample_buffer->setFilter(
         osg::Texture::FilterParameter::MIN_FILTER,
         osg::Texture::FilterMode::NEAREST);
      multisample_buffer->setFilter(
         osg::Texture::FilterParameter::MAG_FILTER,
         osg::Texture::FilterMode::NEAREST);
      multisample_buffer->setResizeNonPowerOfTwoHint(false);

      multisample_buffer->apply(
         *graphics_context_->getState());
      
      std::cout
         << "Multisample ID = "
         << multisample_buffer->getTextureObject(
               graphics_context_->getState()->getContextID())->id()
         << std::endl;
   }

   return multisample_buffer;
}

void OSGView::UpdateText(
   const GLuint color_buffer_texture_id ) const noexcept
{
   const auto scene_data =
      osg_scene_view_->getSceneData();

   const auto text_node =
      scene_data->asTransform()->getChild(1);

   static_cast< osgText::Text * >(
      text_node)->setText(
         std::to_string(color_buffer_texture_id));
}

std::pair< bool, GLuint > OSGView::SetupNextFrame( ) noexcept
{
   std::pair< bool, GLuint > setup { false, 0 };

   ProcessWaitingFenceSyncs();

   const auto frame_buffer =
      GetNextFrameBuffer();

   if (frame_buffer.second)
   {
      const auto color_texture =
         const_cast< osg::Texture2D * >(
            static_cast< const osg::Texture2D * >(
               frame_buffer.second->getAttachment(
                  osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0).getTexture()));

      if (color_texture->getTextureWidth() != width_ ||
          color_texture->getTextureHeight() != height_)
      {
         const auto subload_callback =
            static_cast< FrameBufferSubloadCallback * >(
               color_texture->getSubloadCallback());

         subload_callback->SetPerformingResize(true);

         color_texture->setTextureSize(
            width_,
            height_);

         color_texture->apply(
            *graphics_context_->getState());

         subload_callback->SetPerformingResize(false);

         osg_scene_view_->getCamera()->setProjectionMatrix(
            osg::Matrix::perspective(
               45.0,
               static_cast< double >(width_) /
               static_cast< double >(height_),
               1.0, 200.0));

         osg_scene_view_->getViewport()->setViewport(
            0.0, 0.0,
            width_, height_);
      }

      if (multisample_frame_buffer_)
      {
         const auto multisample_color_texture =
         const_cast< osg::Texture2DMultisample * >(
            static_cast< const osg::Texture2DMultisample * >(
               multisample_frame_buffer_->getAttachment(
                  osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0).getTexture()));

         if (multisample_color_texture->getTextureWidth() != width_ ||
             multisample_color_texture->getTextureHeight() != height_)
         {
            multisample_color_texture->setTextureSize(
               width_,
               height_);

            multisample_color_texture->apply(
               *graphics_context_->getState());

            graphics_context_->getState()->get< osg::GLExtensions >(
               )->glTexImage2DMultisample(
                     multisample_color_texture->getTextureTarget(),
                     multisample_color_texture->getNumSamples(),
                     multisample_color_texture->getInternalFormat(),
                     width_, height_,
                     multisample_color_texture->getFixedSampleLocations());
         }
      }

      if (multisample_frame_buffer_)
      {
#if USE_SINGLE_DEPTH_STENCIL_MULTISAMPLE_ATTACHMENT
         const osg::FrameBufferObject::BufferComponent attachments[] {
            osg::FrameBufferObject::BufferComponent::PACKED_DEPTH_STENCIL_BUFFER
         };
#else
         const osg::FrameBufferObject::BufferComponent attachments[] {
            osg::FrameBufferObject::BufferComponent::DEPTH_BUFFER,
            osg::FrameBufferObject::BufferComponent::STENCIL_BUFFER
         };
#endif

         for (const auto attachment : attachments)
         {
            const auto attachment_texture =
               const_cast< osg::Texture2DMultisample * >(
                  static_cast< const osg::Texture2DMultisample* >(
                     multisample_frame_buffer_->getAttachment(
                        attachment).getTexture()));
            
            if (attachment_texture->getTextureWidth() != width_ ||
                attachment_texture->getTextureHeight() != height_)
            {
               attachment_texture->setTextureSize(
                  width_,
                  height_);
            
               attachment_texture->apply(
                  *graphics_context_->getState());

               graphics_context_->getState()->get< osg::GLExtensions >(
                  )->glTexImage2DMultisample(
                        attachment_texture->getTextureTarget(),
                        attachment_texture->getNumSamples(),
                        attachment_texture->getInternalFormat(),
                        width_, height_,
                        attachment_texture->getFixedSampleLocations());
            }
         }
      }
      else
      {
         const auto depth_buffer =
            const_cast< osg::Texture2D * >(
               static_cast< const osg::Texture2D * >(
                  frame_buffer.second->getAttachment(
                     osg::FrameBufferObject::BufferComponent::PACKED_DEPTH_STENCIL_BUFFER).getTexture()));
         
         if (depth_buffer->getTextureWidth() != width_ ||
             depth_buffer->getTextureHeight() != height_)
         {
            const auto subload_callback =
               static_cast< FrameBufferSubloadCallback * >(
                  depth_buffer->getSubloadCallback());
         
            subload_callback->SetPerformingResize(true);
         
            depth_buffer->setTextureSize(
               width_,
               height_);
         
            depth_buffer->apply(
               *graphics_context_->getState());
         
            subload_callback->SetPerformingResize(false);
         }
      }

      if (multisample_frame_buffer_)
         osg_scene_view_->getRenderStage()->setMultisampleResolveFramebufferObject(
            frame_buffer.second);
      else
         osg_scene_view_->getRenderStage()->setFrameBufferObject(
            frame_buffer.second);

      setup.first = true;
      setup.second = frame_buffer.first;
   }

   return setup;
}

std::pair< GLuint, osg::ref_ptr< osg::FrameBufferObject > >
OSGView::GetNextFrameBuffer( ) noexcept
{
   std::pair< GLuint, osg::ref_ptr< osg::FrameBufferObject > >
      frame_buffer { 0, nullptr };

   if (!inactive_frame_buffers_.empty())
   {
      const auto frame_buffer_it = inactive_frame_buffers_.cbegin();
      frame_buffer.first = frame_buffer_it->first;
      frame_buffer.second = frame_buffer_it->second;

#if _has_cxx_std_map_extract
      active_frame_buffers_.insert(
         inactive_frame_buffers_.extract(
            frame_buffer_it->first));
#else
      active_frame_buffers_.insert({
         frame_buffer_it->first,
         frame_buffer_it->second});

      inactive_frame_buffers_.erase(
         frame_buffer_it);
#endif
   }

   return frame_buffer;
}

void OSGView::ProcessWaitingFenceSyncs( ) noexcept
{
   const auto removed =
      std::remove_if(
         active_fence_syncs_.begin(),
         active_fence_syncs_.end(),
         [ this ] ( const auto & fence_sync )
         {
            bool signaled {
               fence_sync.second->IsSignaled() };

            if (signaled)
            {
               emit
                  Present(fence_sync.first);
            }

            return signaled;
         });

   active_fence_syncs_.erase(
      removed,
      active_fence_syncs_.end());
}

void OSGView::OnResize(
   const int32_t width,
   const int32_t height ) noexcept
{
   width_ = static_cast< uint32_t >(width);
   height_ = static_cast< uint32_t >(height);
}
