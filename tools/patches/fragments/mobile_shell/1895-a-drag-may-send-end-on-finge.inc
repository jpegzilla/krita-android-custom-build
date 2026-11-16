    // A drag may send end() on finger release and again on TouchEnd.
#ifdef Q_OS_ANDROID
    const int shortcut = m_shortcut;
    const bool anchorAtTap=m_mobileTap;
    m_mobileTap=false;
    std::optional<QPointF> mirrorTap;
    if (anchorAtTap && shortcut==MirrorView && event) {
        const auto *touch=static_cast<QTouchEvent*>(event);
        if (!touch->touchPoints().isEmpty()) {
            QPointF center;
            for (const auto &point:touch->touchPoints()) center+=point.pos();
            mirrorTap=center/touch->touchPoints().size();
        }
    }
#endif
    m_shortcut = -1;
    if (actionName.isEmpty()) return;

    auto *manager = inputManager();
    QPointer<KisCanvas2> canvas = manager ? manager->canvas() : nullptr;
    auto *view = canvas ? canvas->viewManager() : nullptr;
    if (!view) return;
#ifdef Q_OS_ANDROID
    if (!view->mainWindow()) {
        switch (shortcut) {
        case UndoActionShortcut: actionName = QStringLiteral("mobile_gesture_undo"); break;
        case RedoActionShortcut: actionName = QStringLiteral("mobile_gesture_redo"); break;
        case ToggleCanvasOnlyShortcut: actionName = QStringLiteral("mobile_canvas_only"); break;
        default: break;
        }
    }
#endif
    // Undo, layer changes and tool switches can reenter the input matcher.
    // Dispatch after it finishes the current gesture and verify the canvas.
    QTimer::singleShot(0, view, [view, canvas, actionName
#ifdef Q_OS_ANDROID
                                , mirrorTap
#endif
                               ]() {
        if (!canvas || view->canvasBase() != canvas.data()) return;
#ifdef Q_OS_ANDROID
        if (mirrorTap && !view->mainWindow()) {
            if (auto *controller=dynamic_cast<KisCanvasController*>(canvas->canvasController())) {
                controller->mobileMirrorAtTap(*mirrorTap);
                return;
            }
        }
#endif
        auto *collection = view->actionCollection();
        auto *action = collection->action(actionName);
        if (!action) {
            const QString family = actionName.section(QLatin1Char('/'), -1);
            if (family.startsWith(QStringLiteral("KisTool"))) {
                for (auto *candidate : collection->actions()) {
                    if (candidate->objectName().section(QLatin1Char('/'), -1) == family) {
                        action = candidate;
                        break;
                    }
                }
            }
        }
        if (action && action->isEnabled()) action->trigger();
    });