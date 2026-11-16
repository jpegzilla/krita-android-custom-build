    connect(quickMenu, &QMenu::aboutToShow, this,
            [this, quickMenu, customizeQuick, tabletQuickDefaults]() {
        quickMenu->clear();
        const QStringList selected = QSettings().value(
            QStringLiteral("TabletUi/quickActionsV2"), tabletQuickDefaults).toStringList();
        for (const QString &name : selected) {
            QAction *action = actionCollection()->action(name);
            if (action) quickMenu->addAction(action);
        }
        quickMenu->addSeparator();
        // Flatten one level: native menu entries directly in Quick Actions.
        for(QAction *topLevel:menuBar()->actions()) {
            if(QMenu *native=topLevel->menu())
                quickMenu->addMenu(native);
        }
        quickMenu->addSeparator();
        quickMenu->addAction(customizeQuick);
    });

    QToolBar *tabletTransformBar = new QToolBar(i18n("Transform Actions"), this);
    tabletTransformBar->setObjectName(QStringLiteral("tabletTransformContextBar"));
    tabletTransformBar->setMovable(false);
    tabletTransformBar->setFloatable(false);
    tabletTransformBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    auto *transformModeButton = new QToolButton(tabletTransformBar);
    transformModeButton->setText(i18n("Mode"));
    transformModeButton->setPopupMode(QToolButton::InstantPopup);
    auto *transformModeMenu = new QMenu(transformModeButton);
    auto getTransformTool = [this]() -> KoToolBase* {
        KisCanvas2 *canvas = d->viewManager->canvasBase();
        if (!canvas) return nullptr;
        return KoToolManager::instance()->toolById(canvas, QStringLiteral("KisToolTransform"));
    };
    const QStringList transformNames = {
        i18n("Free"), i18n("Warp"), i18n("Cage"),
        i18n("Liquify"), i18n("Perspective"), i18n("Mesh")};
    for (int mode = 0; mode < transformNames.size(); ++mode) {
        QAction *choice = transformModeMenu->addAction(transformNames.at(mode));
        connect(choice, &QAction::triggered, this, [getTransformTool, mode]() {
            if (KoToolBase *tool = getTransformTool())
                tool->setProperty("transformMode", mode);
        });
    }
    transformModeButton->setMenu(transformModeMenu);
    tabletTransformBar->addWidget(transformModeButton);
    QAction *resetTransform = tabletTransformBar->addAction(i18n("Reset"));
    connect(resetTransform, &QAction::triggered, this, [getTransformTool]() {
        if (KoToolBase *tool = getTransformTool())
            QMetaObject::invokeMethod(tool, "slotRestartTransform", Qt::DirectConnection);
    });
    QAction *cancelTransform = tabletTransformBar->addAction(i18n("Cancel"));
    connect(cancelTransform, &QAction::triggered, this, [getTransformTool]() {
        if (KoToolBase *tool = getTransformTool())
            QMetaObject::invokeMethod(tool, "slotCancelTransform", Qt::DirectConnection);
    });
    QAction *applyTransform = tabletTransformBar->addAction(i18n("Apply"));
    connect(applyTransform, &QAction::triggered, this, [getTransformTool]() {
        if (KoToolBase *tool = getTransformTool())
            QMetaObject::invokeMethod(tool, "applyTransform", Qt::DirectConnection);
    });
    addToolBar(Qt::BottomToolBarArea, tabletTransformBar);
    tabletTransformBar->hide();
    auto updateTabletTransformBar = [this, tabletCompact, tabletTransformBar]() {
        const bool active = d->viewManager->canvasBase() &&
            KoToolManager::instance()->activeToolId() == QStringLiteral("KisToolTransform");
        tabletTransformBar->setVisible(tabletCompact->isChecked() && active);
    };
    connect(KoToolManager::instance(), &KoToolManager::changedTool, this,
            [updateTabletTransformBar](KoCanvasController *) {
        updateTabletTransformBar();
    });
    connect(tabletCompact, &QAction::toggled, this,
            [updateTabletTransformBar](bool) { updateTabletTransformBar(); });
