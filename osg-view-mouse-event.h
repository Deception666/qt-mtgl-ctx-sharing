#ifndef _OSG_VIEW_MOUSE_EVENT_H_
#define _OSG_VIEW_MOUSE_EVENT_H_

#include <QtGui/QMouseEvent>

#include <cstdint>

extern const int32_t MOUSE_MOVE_EVENT;

class MouseEvent :
   public QMouseEvent
{
public:
   MouseEvent(
      const Type event_type,
      const QMouseEvent & event ) noexcept;

};

#endif // _OSG_VIEW_MOUSE_EVENT_H_
