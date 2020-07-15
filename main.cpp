#include "qt-gl-view.h"
#include "render-thread.h"

#if _WIN32
#include <QtPlatformHeaders/QWGLNativeContext>
#endif // _WIN32

#include <QtWidgets/QApplication>

#include <QtGui/QOpenGLContext>

#include <QtCore/QCoreApplication>
#include <QtCore/QVariant>

#include <Qt>

#if _has_cxx_std_any
#include <any>
#else
#include "stl-ext/any"
#endif

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include <string.h>

#define DISPLAY_ONLY_ONE_VIEW 0

auto GetCommandLineArgs(
   const int32_t argc,
   const char * const * const argv )
{
   std::vector< char * > arg_ptrs;
   std::vector<
      std::unique_ptr< char, decltype(&std::free) > >
      arg_strs;

   std::for_each(
      argv,
      argv + argc,
      [ & ] ( const auto arg )
      {
         arg_strs.emplace_back(
               strdup(arg),
               &std::free);
         arg_ptrs.emplace_back(
            arg_strs.back().get());
      });

   arg_ptrs.emplace_back(
      nullptr);

   return
      std::make_pair(
         std::move(arg_ptrs),
         std::move(arg_strs));
}

std::any SetupHiddenGLContextFromGlobalQtGLContext( ) noexcept
{
   std::any hidden_gl_context;

   const auto qt_gl_context =
      QOpenGLContext::globalShareContext();

   if (qt_gl_context)
   {
      const auto native_handle =
         qt_gl_context->nativeHandle();

#if _WIN32
      hidden_gl_context.swap(
         std::any {
            std::make_tuple(
               HWND { },
               HDC { },
               qvariant_cast< QWGLNativeContext >(
                  native_handle).context()) });
#else
#error "Define for your platform!"
#endif
   }

   return hidden_gl_context;
}

int32_t main(
   const int32_t argc,
   const char * const (& argv)[] )
{
   QCoreApplication::setAttribute(
      Qt::AA_ShareOpenGLContexts,
      true);

   auto _argc { argc };
   auto _argv { GetCommandLineArgs(argc, argv) };

   QApplication application {
      _argc, _argv.first.data() };

   render_thread::Start(
      SetupHiddenGLContextFromGlobalQtGLContext());

   std::vector<
      std::unique_ptr< QtGLView > > gl_views;

#if !DISPLAY_ONLY_ONE_VIEW
   for (size_t i { 0 }; i < 2; ++i)
   {
#endif
      gl_views.emplace_back(
         std::make_unique< QtGLView >(
            "CopCapr.IVE", nullptr));
      gl_views.back()->show();
      gl_views.back()->SetCameraLookAt(
         { 5.0, 5.0, 2.5 },
         { 0.0, 0.0, 0.0 },
         { 0.0, 0.0, 1.0 });

#if !DISPLAY_ONLY_ONE_VIEW
      gl_views.emplace_back(
         std::make_unique< QtGLView >(
            "ElectEng.IVE", nullptr));
      gl_views.back()->show();
      gl_views.back()->SetCameraLookAt(
         { 15.0, 15.0, 10.0 },
         { 0.0, 0.0, 0.0 },
         { 0.0, 0.0, 1.0 });

      gl_views.emplace_back(
         std::make_unique< QtGLView >(
            "T72.ive", nullptr));
      gl_views.back()->show();
      gl_views.back()->SetCameraLookAt(
         { 10.0, 10.0, 5.0 },
         { 0.0, 0.0, 0.0 },
         { 0.0, 0.0, 1.0 });

      gl_views.emplace_back(
         std::make_unique< QtGLView >(
            "Sub_LAclass.IVE", nullptr));
      gl_views.back()->show();
      gl_views.back()->SetCameraLookAt(
         { 25.0, 55.0, 25.0 },
         { 0.0, 0.0, 0.0 },
         { 0.0, 0.0, 1.0 });

      gl_views.emplace_back(
         std::make_unique< QtGLView >(
            "A10.ive", nullptr));
      gl_views.back()->show();
      gl_views.back()->SetCameraLookAt(
         { 15.0, 15.0, 10.0 },
         { 0.0, 0.0, 0.0 },
         { 0.0, 0.0, 1.0 });
   }
#endif

   const int32_t exit_code =
      QApplication::exec();

   gl_views.clear();

   render_thread::Stop();

   return exit_code;
}
