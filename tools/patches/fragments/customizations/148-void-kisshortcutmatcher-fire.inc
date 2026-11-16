void KisShortcutMatcher::fireReadyTouchShortcut(QTouchEvent *event, bool bypassDoubleTap)
{
    KisTouchShortcut *doubleTap = nullptr;
    if (!bypassDoubleTap && !m_d->isTouchHeld && !m_d->isTouchDragDetected &&
        event->touchPoints().size() == 1 && !m_d->actionsSuppressedIgnoreFocus()) {
        for (KisTouchShortcut *shortcut : m_d->touchShortcuts) {
            if (shortcut->matchDoubleTapType(event) &&
                shortcut->isAvailable(m_d->actionGroupMask()) &&
                (!doubleTap || shortcut->priority() > doubleTap->priority())) {
                doubleTap = shortcut;
            }
        }
    }
    if (doubleTap) {
        const QPointF point = event->touchPoints().first().pos();
        const qreal threshold = qMax(qreal(24), qreal(QApplication::startDragDistance()) * 2);
        const QPointF delta = point - m_d->lastOneFingerTapPosition;
        const bool nearby = delta.x() * delta.x() + delta.y() * delta.y() <= threshold * threshold;
        if (m_d->pendingSingleTapTimer.isActive() &&
            m_d->lastOneFingerTapAge.isValid() &&
            m_d->lastOneFingerTapAge.elapsed() <= QApplication::doubleClickInterval() && nearby) {
            m_d->pendingSingleTapTimer.stop();
            m_d->pendingSingleTapEvent.reset();
            m_d->lastOneFingerTapAge.invalidate();
            doubleTap->action()->activate(doubleTap->shortcutIndex());
            doubleTap->action()->begin(doubleTap->shortcutIndex(), event);
            doubleTap->action()->end(event);
            doubleTap->action()->deactivate(doubleTap->shortcutIndex());
            return;
        }
        if (m_d->pendingSingleTapEvent) {
            m_d->pendingSingleTapTimer.stop();
            auto *queued = static_cast<QTouchEvent *>(m_d->pendingSingleTapEvent.data());
            fireReadyTouchShortcut(queued, true);
            m_d->pendingSingleTapEvent.reset();
        }
        m_d->lastOneFingerTapPosition = point;
        m_d->lastOneFingerTapAge.start();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        KoPointerEvent::copyQtPointerEvent(event, m_d->pendingSingleTapEvent);
#else
        m_d->pendingSingleTapEvent.reset(event->clone());
#endif
        m_d->pendingSingleTapTimer.start(QApplication::doubleClickInterval());
        return;
    }
    KisTouchShortcut *candidate = nullptr;
    if (bypassDoubleTap) {
        for (KisTouchShortcut *shortcut : m_d->touchShortcuts) {
            if (shortcut->matchTapType(event) &&
                shortcut->isAvailable(m_d->actionGroupMask()) &&
                (!candidate || shortcut->priority() > candidate->priority())) {
                candidate = shortcut;
            }
        }
    } else {
        candidate = matchTouchShortcut(event);
    }
    if (candidate && !m_d->actionsSuppressedIgnoreFocus()) {
        candidate->action()->activate(candidate->shortcutIndex());
        candidate->action()->begin(candidate->shortcutIndex(), event);
        candidate->action()->end(event);
        candidate->action()->deactivate(candidate->shortcutIndex());
    }
}
