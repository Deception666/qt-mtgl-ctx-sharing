#ifndef _RENDER_THREAD_H_
#define _RENDER_THREAD_H_

#if _has_cxx_std_any
#include <any>
#else
#include "stl-ext/any"
#endif

#include <functional>
#include <future>
#include <memory>

class OSGView;

namespace render_thread
{

void Start(
   std::any hidden_gl_context ) noexcept;
void Stop( ) noexcept;

bool RegisterOSGView(
   std::weak_ptr< OSGView > osg_view ) noexcept;
bool UnregisterOSGView(
   std::weak_ptr< OSGView > osg_view ) noexcept;

std::future< void > AddOperation(
   std::function< void ( ) > operation ) noexcept;
std::future< void > AddOperation(
   std::function< void ( const std::any & ) > operation,
   std::any arguments ) noexcept;

} // namespace rt

#endif // _RENDER_THREAD_H_
