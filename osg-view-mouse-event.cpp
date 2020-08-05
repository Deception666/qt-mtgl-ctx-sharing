#include "osg-view-mouse-event.h"

#include <QEvent>

const int32_t MOUSE_MOVE_EVENT =
   QEvent::registerEventType();

MouseEvent::MouseEvent(
   const Type event_type,
   const QMouseEvent & event ) noexcept :
QMouseEvent(
   event_type,
   event.localPos(),
   event.windowPos(),
   event.screenPos(),
   event.button(),
   event.buttons(),
   event.modifiers(),
   event.source())
{
}
