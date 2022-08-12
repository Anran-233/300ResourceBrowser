#ifndef CEVENTFILTER_H
#define CEVENTFILTER_H

#include <QKeyEvent>

// 事件过滤器（屏蔽Dialog自带的Esc按键事件）
class CEventFilter : public QObject {
private:
    bool eventFilter( QObject *, QEvent *event ) override {
        if ( event->type() == QEvent::KeyPress ||
            event->type() == QEvent::KeyRelease ) {
            if ( ( (QKeyEvent *) event )->key() == Qt::Key_Escape ) {
                return true;
            }
        }
        return false;
    }
};
static CEventFilter sg_cEventFilter;

#endif // CEVENTFILTER_H
