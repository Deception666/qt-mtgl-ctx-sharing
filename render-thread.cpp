#include "render-thread.h"
#include "osg-view.h"

#include <QtCore/QEventLoop>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

extern void InitHiddenGLContext(
   const std::any & hidden_context ) noexcept;
extern void ReleaseHiddenGLContext( ) noexcept;

namespace render_thread
{

std::thread render_thread_;
std::mutex render_thread_mutex_;

std::atomic_bool quit_render_thread_ { false };

std::deque<
   std::function< void ( ) > > operations_;
std::mutex operations_mutex_;

std::list<
   std::weak_ptr< OSGView > > osg_views_;
std::mutex osg_views_mutex_;

size_t ExecuteOperation( )
{
#if _has_cxx_class_template_argument_deduction
   std::lock_guard lock {
      operations_mutex_ };
#else
   std::lock_guard< decltype(operations_mutex_) > lock {
      operations_mutex_ };
#endif

   if (!operations_.empty())
   {
      operations_.front()();

      operations_.pop_front();
   }

   return operations_.size();
}

void RenderOSGViews( )
{
#if _has_cxx_class_template_argument_deduction
   std::lock_guard lock {
      osg_views_mutex_ };
#else
   std::lock_guard< decltype(osg_views_mutex_) > lock {
      osg_views_mutex_ };
#endif

   for (const auto & osg_view : osg_views_)
   {
      const auto shared_osg_view =
         osg_view.lock();

      if (shared_osg_view)
      {
         shared_osg_view->PreRender();
         shared_osg_view->Render();
         shared_osg_view->PostRender();
      }
   }
}

void RenderLoop( )
{
   while (!quit_render_thread_)
   {
      const auto time_start =
         std::chrono::steady_clock::now();

      ExecuteOperation();

      QEventLoop().processEvents(
         QEventLoop::AllEvents);

      RenderOSGViews();

      const auto time_end =
         std::chrono::steady_clock::now();

      const auto time_delta =
         time_end - time_start;

      const std::chrono::milliseconds
         frame_rate { 33 };

      if (time_delta < frame_rate)
      {
         std::this_thread::sleep_for(
            frame_rate - time_delta);
      }

      std::cout
         << "Frame time "
         << std::chrono::duration_cast<
               std::chrono::milliseconds >(time_delta).count()
         << " ms"
         << std::endl;
   }

   while (ExecuteOperation());
}

void Start(
   std::any hidden_gl_context ) noexcept
{
#if _has_cxx_class_template_argument_deduction
   std::lock_guard lock {
      render_thread_mutex_ };
#else
   std::lock_guard< decltype(render_thread_mutex_) > lock {
      render_thread_mutex_ };
#endif

   if (render_thread_.get_id() ==
       std::thread::id { })
   {
      render_thread_ =
         std::thread {
            &RenderLoop };

      AddOperation(
         &InitHiddenGLContext,
         std::move(hidden_gl_context)).wait();
   }
}

void Stop( ) noexcept
{
#if _has_cxx_class_template_argument_deduction
   std::lock_guard lock {
      render_thread_mutex_ };
#else
   std::lock_guard< decltype(render_thread_mutex_) > lock {
      render_thread_mutex_ };
#endif

   if (render_thread_.get_id() !=
       std::thread::id { })
   {
      AddOperation(
         &ReleaseHiddenGLContext).wait();

      quit_render_thread_ = true;

      render_thread_.join();
   }
}

bool RegisterOSGView(
   std::weak_ptr< OSGView > osg_view ) noexcept
{
#if _has_cxx_class_template_argument_deduction
   std::lock_guard lock {
      osg_views_mutex_ };
#else
   std::lock_guard< decltype(osg_views_mutex_) > lock {
      osg_views_mutex_ };
#endif

   const auto registered_view =
      std::find_if(
         osg_views_.cbegin(),
         osg_views_.cend(),
         [ shared_osg_view = osg_view.lock() ] (
            const auto & view )
         {
            const auto shared_view =
               view.lock();

            return
               shared_view &&
               shared_osg_view &&
               shared_view == shared_osg_view;
         });

   return
      registered_view == osg_views_.cend() &&
      osg_views_.emplace(
         osg_views_.cend(),
         std::move(osg_view)) !=
      osg_views_.cend();
}

bool UnregisterOSGView(
   std::weak_ptr< OSGView > osg_view ) noexcept
{
#if _has_cxx_class_template_argument_deduction
   std::lock_guard lock {
      osg_views_mutex_ };
#else
   std::lock_guard< decltype(osg_views_mutex_) > lock {
      osg_views_mutex_ };
#endif

   const auto current_size =
      osg_views_.size();

   osg_views_.remove_if(
      [ shared_osg_view = osg_view.lock() ] (
         const auto & view )
      {
         const auto shared_view =
            view.lock();

         return
            shared_view &&
            shared_osg_view &&
            shared_view == shared_osg_view;
      });

   return current_size != osg_views_.size();
}

std::future< void > AddOperation(
   std::function< void ( ) > operation ) noexcept
{
#if _has_cxx_class_template_argument_deduction
    std::lock_guard lock {
      operations_mutex_ };
#else
   std::lock_guard< decltype(operations_mutex_) > lock {
      operations_mutex_ };
#endif

   auto complete =
      std::make_shared< std::promise< void > >();

   auto completed =
      complete->get_future();

   operations_.push_back(
      [ complete = std::move(complete),
        operation = std::move(operation) ] ( ) mutable
      {
         operation();

         complete->set_value();
      });

   return completed;
}

std::future< void > AddOperation(
   std::function< void ( const std::any & ) > operation,
   std::any arguments ) noexcept
{
#if _has_cxx_class_template_argument_deduction
    std::lock_guard lock {
      operations_mutex_ };
#else
   std::lock_guard< decltype(operations_mutex_) > lock {
      operations_mutex_ };
#endif

   auto complete =
      std::make_shared< std::promise< void > >();

   auto completed =
      complete->get_future();

   operations_.push_back(
      [ complete = std::move(complete),
        operation = std::move(operation),
        arguments = std::move(arguments) ] ( ) mutable
      {
         operation(arguments);

         complete->set_value();
      });

   return completed;
}

} // namespace render_thread
