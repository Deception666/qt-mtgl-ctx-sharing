#ifndef _RENDER_THREAD_H_
#define _RENDER_THREAD_H_

#include <functional>
#include <future>
#include <memory>

class OSGView;

namespace render_thread
{

void Start( ) noexcept;
void Stop( ) noexcept;

bool RegisterOSGView(
   std::weak_ptr< OSGView > osg_view ) noexcept;
bool UnregisterOSGView(
   std::weak_ptr< OSGView > osg_view ) noexcept;

std::future< void > AddOperation(
   std::function< void ( ) > operation ) noexcept;

} // namespace rt

#endif // _RENDER_THREAD_H_
