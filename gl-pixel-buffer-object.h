#ifndef _GL_PIXEL_BUFFER_OBJECT_H_
#define _GL_PIXEL_BUFFER_OBJECT_H_

#include <cstdint>
#include <vector>

namespace gl
{

class PixelBufferObject final
{
public:
   enum class Operation
   {
      NONE, PACK, UNPACK
   };

   PixelBufferObject( ) noexcept;
   ~PixelBufferObject( ) noexcept;

   PixelBufferObject( PixelBufferObject && o ) noexcept;
   PixelBufferObject( const PixelBufferObject & ) noexcept = delete;

   PixelBufferObject & operator = ( PixelBufferObject && o ) noexcept;
   PixelBufferObject & operator = ( const PixelBufferObject & ) noexcept = delete;

   bool IsValid( ) const noexcept;

   void Bind(
      const Operation op ) noexcept;

   bool IsBound( ) const noexcept;

   void SetSize(
      const uint32_t size_in_bytes ) noexcept;

   uint32_t GetSize( ) const noexcept;

   template < typename T >
   std::vector< T > ReadData(
      const size_t offset,
      const size_t size ) const noexcept;

   bool ReadData(
      const size_t offset,
      const size_t size_in_bytes,
      void * const data ) const noexcept;

   bool WriteData(
      const size_t offset,
      const size_t size_in_bytes,
      void * const data ) const noexcept;

private:
   Operation current_operation_;

   uint32_t buffer_size_bytes_;

   const uint32_t pixel_buffer_object_;

};

template < typename T >
std::vector< T > PixelBufferObject::ReadData(
   const size_t offset,
   const size_t size ) const noexcept
{
   std::vector< T > data;

   const size_t size_in_bytes =
      size * sizeof(T);

   if (size_in_bytes && IsBound())
   {
      data.resize(size);

      const bool read =
         ReadData(
            offset,
            size_in_bytes,
            data.data());

      if (!read)
         data.clear();
   }

   return data;
}

} // namespace gl

#endif // _GL_PIXEL_BUFFER_OBJECT_H_
