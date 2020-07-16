#ifndef _GL_FENCE_SYNC_H_
#define _GL_FENCE_SYNC_H_

namespace gl
{

class FenceSync final
{
public:
   FenceSync( ) noexcept;
   ~FenceSync( ) noexcept;

   FenceSync( FenceSync && o ) noexcept;
   FenceSync( const FenceSync & ) noexcept = delete;

   FenceSync & operator = ( FenceSync && ) noexcept;
   FenceSync & operator = ( const FenceSync & ) noexcept = delete;

   bool Valid( ) const noexcept;
   bool IsSignaled( ) const noexcept;

private:
   void * const fence_sync_;

};

} // namespace gl

#endif // _GL_FENCE_SYNC_H_
