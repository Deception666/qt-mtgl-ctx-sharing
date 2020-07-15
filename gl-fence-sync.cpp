#include "gl-fence-sync.h"

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GL/GL.h>

#include <algorithm>
#include <cassert>
#include <cstdint>

#define GL_SYNC_CONDITION                 0x9113
#define GL_SYNC_STATUS                    0x9114
#define GL_SYNC_FLAGS                     0x9115
#define GL_SYNC_FENCE                     0x9116
#define GL_SYNC_GPU_COMMANDS_COMPLETE     0x9117
#define GL_UNSIGNALED                     0x9118
#define GL_SIGNALED                       0x9119

namespace gl
{

namespace ext
{

using GLsync = void *;
using GLuint64 = uint64_t;
using GLint64 = int64_t;

static GLsync (APIENTRY *glFenceSync)(GLenum condition, GLbitfield flags) { nullptr };
static GLboolean (APIENTRY *glIsSync)(GLsync sync) { nullptr };
static void (APIENTRY *glDeleteSync)(GLsync sync) { nullptr };
static GLenum (APIENTRY *glClientWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout) { nullptr };
static void (APIENTRY *glWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout) { nullptr };
static void (APIENTRY *glGetSynciv)(GLsync sync, GLenum pname, GLsizei count, GLsizei *length, GLint *values) { nullptr };

} // namespace ext

static bool SetupExtensions( )
{
#if _WIN32
   assert(wglGetCurrentContext());
   
   if (!ext::glFenceSync || !ext::glIsSync ||
       !ext::glDeleteSync || !ext::glClientWaitSync ||
       !ext::glWaitSync || !ext::glGetSynciv)
   {
      ext::glFenceSync =
         reinterpret_cast< decltype(ext::glFenceSync) >(
            wglGetProcAddress("glFenceSync"));
      ext::glIsSync =
         reinterpret_cast< decltype(ext::glIsSync) >(
            wglGetProcAddress("glIsSync"));
      ext::glDeleteSync =
         reinterpret_cast< decltype(ext::glDeleteSync) >(
            wglGetProcAddress("glDeleteSync"));
      ext::glClientWaitSync =
         reinterpret_cast< decltype(ext::glClientWaitSync) >(
            wglGetProcAddress("glClientWaitSync"));
      ext::glWaitSync =
         reinterpret_cast< decltype(ext::glWaitSync) >(
            wglGetProcAddress("glWaitSync"));
      ext::glGetSynciv =
         reinterpret_cast< decltype(ext::glGetSynciv) >(
            wglGetProcAddress("glGetSynciv"));
   }

   return
      ext::glFenceSync && ext::glIsSync &&
      ext::glDeleteSync && ext::glClientWaitSync &&
      ext::glWaitSync && ext::glGetSynciv;
#else
#error "Define for this platform!"
#endif
}

static void * CreateFenceSync( )
{
   void * fence_sync { nullptr };

   if (SetupExtensions())
   {
      fence_sync =
         ext::glFenceSync(
            GL_SYNC_GPU_COMMANDS_COMPLETE,
            0);
   }

   return fence_sync;
}

FenceSync::FenceSync( ) noexcept :
fence_sync_ { CreateFenceSync() }
{
}

FenceSync::~FenceSync( ) noexcept
{
   if (fence_sync_)
   {
      ext::glDeleteSync(
         fence_sync_);
   }
}

FenceSync::FenceSync(
   FenceSync && o ) noexcept :
fence_sync_ { nullptr }
{
   std::swap(
      const_cast< ext::GLsync >(fence_sync_),
      const_cast< ext::GLsync >(o.fence_sync_));
}

FenceSync & FenceSync::operator = (
   FenceSync && o ) noexcept
{
   if (&o != this)
   {
      std::swap(
         const_cast< ext::GLsync >(fence_sync_),
         const_cast< ext::GLsync >(o.fence_sync_));
   }

   return *this;
}

bool FenceSync::Valid( ) const noexcept
{
   return fence_sync_;
}

bool FenceSync::IsSignaled( ) const noexcept
{
   bool signaled { false };

   if (Valid())
   {
      GLint value { 0 };
      GLsizei inserted { 0 };

      ext::glGetSynciv(
         fence_sync_,
         GL_SYNC_STATUS,
         1,
         &inserted,
         &value);

      if (inserted == 1)
      {
         signaled =
            GL_SIGNALED == value;
      }
   }

   return signaled;
}

} // namespace gl
