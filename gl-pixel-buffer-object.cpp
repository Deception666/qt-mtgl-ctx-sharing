#include "gl-pixel-buffer-object.h"

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GL/GL.h>

#include <algorithm>
#include <cassert>
#include <memory>

#define GL_BUFFER_SIZE                    0x8764

#define GL_READ_ONLY                      0x88B8
#define GL_WRITE_ONLY                     0x88B9
#define GL_READ_WRITE                     0x88BA

#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA

#define GL_PIXEL_PACK_BUFFER              0x88EB
#define GL_PIXEL_UNPACK_BUFFER            0x88EC
#define GL_PIXEL_PACK_BUFFER_BINDING      0x88ED
#define GL_PIXEL_UNPACK_BUFFER_BINDING    0x88EF

namespace gl
{

namespace ext
{

using GLsizeiptr = intptr_t;
using GLintptr = intptr_t;

void (APIENTRY * glBindBuffer)(GLenum target, GLuint buffer);
void (APIENTRY * glDeleteBuffers)(GLsizei n, const GLuint *buffers);
void (APIENTRY * glGenBuffers)(GLsizei n, GLuint *buffers);
GLboolean (APIENTRY * glIsBuffer)(GLuint buffer);
void (APIENTRY * glBufferData)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
void (APIENTRY * glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
void (APIENTRY * glGetBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, void *data);
void * (APIENTRY * glMapBuffer)(GLenum target, GLenum access);
GLboolean (APIENTRY * glUnmapBuffer)(GLenum target);
void (APIENTRY * glGetBufferParameteriv)(GLenum target, GLenum pname, GLint *params);
void (APIENTRY * glGetBufferPointerv)(GLenum target, GLenum pname, void **params);

} // namespace ext

template < typename T >
static T GetGLFunc(
   const char * const name,
   T ) noexcept
{
   return
      reinterpret_cast< T >(
         wglGetProcAddress(name));
}

static bool SetupExtensions( )
{
#if _WIN32
   assert(wglGetCurrentContext());
   
   if (!ext::glBindBuffer || !ext::glDeleteBuffers ||
       !ext::glGenBuffers || !ext::glIsBuffer ||
       !ext::glBufferData || !ext::glBufferSubData ||
       !ext::glGetBufferSubData || !ext::glMapBuffer ||
       !ext::glUnmapBuffer || !ext::glGetBufferParameteriv ||
       !ext::glGetBufferPointerv)
   {
      ext::glBindBuffer =
         GetGLFunc("glBindBuffer", ext::glBindBuffer);
      ext::glDeleteBuffers =
         GetGLFunc("glDeleteBuffers", ext::glDeleteBuffers);
      ext::glGenBuffers =
         GetGLFunc("glGenBuffers", ext::glGenBuffers);
      ext::glIsBuffer =
         GetGLFunc("glIsBuffer", ext::glIsBuffer);
      ext::glBufferData =
         GetGLFunc("glBufferData", ext::glBufferData);
      ext::glBufferSubData =
         GetGLFunc("glBufferSubData", ext::glBufferSubData);
      ext::glGetBufferSubData =
         GetGLFunc("glGetBufferSubData", ext::glGetBufferSubData);
      ext::glMapBuffer =
         GetGLFunc("glMapBuffer", ext::glMapBuffer);
      ext::glUnmapBuffer =
         GetGLFunc("glUnmapBuffer", ext::glUnmapBuffer);
      ext::glGetBufferParameteriv =
         GetGLFunc("glGetBufferParameteriv", ext::glGetBufferParameteriv);
      ext::glGetBufferPointerv =
         GetGLFunc("glGetBufferPointerv", ext::glGetBufferPointerv);
   }

   return
      ext::glBindBuffer && ext::glDeleteBuffers &&
      ext::glGenBuffers && ext::glIsBuffer &&
      ext::glBufferData && ext::glBufferSubData &&
      ext::glGetBufferSubData && ext::glMapBuffer &&
      ext::glUnmapBuffer && ext::glGetBufferParameteriv &&
      ext::glGetBufferPointerv;
#else
#error "Define for this platform!"
#endif
}

static uint32_t CreatePixelBufferObject( )
{
   uint32_t pixel_buffer_object_ { 0 };

   if (SetupExtensions())
   {
      ext::glGenBuffers(
         1,
         &pixel_buffer_object_);
   }

   return pixel_buffer_object_;
}

static GLenum TranslateOpToGL(
   const PixelBufferObject::Operation op )
{
   GLenum gl { 0 };

   if (op == PixelBufferObject::Operation::PACK)
      gl = GL_PIXEL_PACK_BUFFER;
   else if (op == PixelBufferObject::Operation::UNPACK)
      gl = GL_PIXEL_UNPACK_BUFFER;
   else
      assert(false);

   return gl;
}

PixelBufferObject::PixelBufferObject( ) noexcept :
current_operation_ { Operation::NONE },
buffer_size_bytes_ { 0 },
pixel_buffer_object_ { CreatePixelBufferObject() }
{
}

PixelBufferObject::~PixelBufferObject( ) noexcept
{
   if (pixel_buffer_object_)
      ext::glDeleteBuffers(
         1,
         &pixel_buffer_object_);
}

PixelBufferObject::PixelBufferObject(
   PixelBufferObject && o ) noexcept :
current_operation_ { o.current_operation_ },
buffer_size_bytes_ { o.buffer_size_bytes_ },
pixel_buffer_object_ { o.pixel_buffer_object_ }
{
   o.current_operation_ = Operation::NONE;
   o.buffer_size_bytes_ = 0;
   const_cast< uint32_t & >(
      o.pixel_buffer_object_) = 0;
}

PixelBufferObject & PixelBufferObject::operator = (
   PixelBufferObject && o ) noexcept
{
   if (&o != this)
   {
      std::swap(
         current_operation_,
         o.current_operation_);
      std::swap(
         buffer_size_bytes_,
         o.buffer_size_bytes_);
      std::swap(
         const_cast< uint32_t & >(pixel_buffer_object_),
         const_cast< uint32_t & >(o.pixel_buffer_object_));
   }

   return *this;
}

bool PixelBufferObject::IsValid( ) const noexcept
{
   return
      pixel_buffer_object_ != 0;
}

void PixelBufferObject::Bind(
   const Operation op ) noexcept
{
   if (op != current_operation_)
   {
      if (Operation::NONE != current_operation_)
      {
         ext::glBindBuffer(
            TranslateOpToGL(current_operation_),
            0);
      }

      if (Operation::NONE != op)
      {
         ext::glBindBuffer(
            TranslateOpToGL(op),
            pixel_buffer_object_);
      }

      current_operation_ = op;
   }
}

bool PixelBufferObject::IsBound( ) const noexcept
{
   // only indicates that this has been bound...
   // it is possible that something else can set
   // the bound target to another buffer object...

   return
      current_operation_ != Operation::NONE;
}

void PixelBufferObject::SetSize(
   const uint32_t size_in_bytes ) noexcept
{
   if (IsBound() &&
       buffer_size_bytes_ != size_in_bytes)
   {
      ext::glBufferData(
         TranslateOpToGL(current_operation_),
         size_in_bytes,
         nullptr,
         current_operation_ == Operation::PACK ?
         GL_DYNAMIC_READ :
         GL_DYNAMIC_DRAW);

      buffer_size_bytes_ = size_in_bytes;
   }
}

uint32_t PixelBufferObject::GetSize( ) const noexcept
{
   return buffer_size_bytes_;
}

bool PixelBufferObject::ReadData(
   const size_t offset,
   const size_t size_in_bytes,
   void * const data ) const noexcept
{
   bool read { false };

   if (size_in_bytes && data && IsBound())
   {
      const auto target =
         TranslateOpToGL(
            current_operation_);

      GLint buffer_size { 0 };
      ext::glGetBufferParameteriv(
         target,
         GL_BUFFER_SIZE,
         &buffer_size);

      if (offset + size_in_bytes <= buffer_size)
      {
         // use std::unique_ptr in the future...
         const auto * const buffer =
            ext::glMapBuffer(
               target,
               GL_READ_ONLY);

         if (buffer)
         {
            std::copy(
               reinterpret_cast< const uint8_t * >(buffer) + offset,
               reinterpret_cast< const uint8_t * >(buffer) + offset + size_in_bytes,
               reinterpret_cast< uint8_t * >(data));

            ext::glUnmapBuffer(
               target);

            read = true;
         }
      }
   }

   return read;
}

bool PixelBufferObject::WriteData(
   const size_t offset,
   const size_t size_in_bytes,
   void * const data ) const noexcept
{
   bool wrote { false };

   if (size_in_bytes && data && IsBound())
   {
      const auto target =
         TranslateOpToGL(
            current_operation_);

      GLint buffer_size { 0 };
      ext::glGetBufferParameteriv(
         target,
         GL_BUFFER_SIZE,
         &buffer_size);

      if (offset + size_in_bytes <= buffer_size)
      {
         // use std::unique_ptr in the future...
         auto * const buffer =
            ext::glMapBuffer(
               target,
               GL_READ_ONLY);

         if (buffer)
         {
            std::copy(
               reinterpret_cast< const uint8_t * >(data) + offset,
               reinterpret_cast< const uint8_t * >(data) + offset + size_in_bytes,
               reinterpret_cast< uint8_t * >(buffer));

            ext::glUnmapBuffer(
               target);

            wrote = true;
         }
      }
   }

   return wrote;
}

} // namespace gl
