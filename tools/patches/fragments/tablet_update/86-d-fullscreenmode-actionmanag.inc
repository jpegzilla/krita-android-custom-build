    d->fullScreenMode = actionManager->createStandardAction(KStandardAction::FullScreen, this, SLOT(viewFullscreen(bool)));

#ifdef Q_OS_ANDROID
    QAction *tabletCompact = new QAction(i18n("Compact UI"), this);
    tabletCompact->setIcon(KisIconUtils::loadIcon("view-fullscreen"));
    tabletCompact->setCheckable(true);
    tabletCompact->setToolTip(i18n("Show or hide menu and document tabs; keep the current workspace and Android immersive mode"));
    actionCollection()->addAction(QStringLiteral("tablet_compact_ui"), tabletCompact);

    connect(tabletCompact, &QAction::toggled, this, [this](bool enabled) {
        QSettings().setValue(QStringLiteral("TabletUi/compact"), enabled);
        if (enabled) {
            const KisConfig previous(true);
            setProperty("tabletPrevSelectionEnabled", previous.selectionActionBar());
            setProperty("tabletPrevSelectionBehavior",
                        static_cast<int>(previous.selectionActionBarBehavior()));
            setProperty("tabletPrevSelectionPosition",
                        static_cast<int>(previous.selectionActionBarPosition()));
            {
                KisConfig settings(false);
                settings.setSelectionActionBar(true);
                settings.setSelectionActionBarBehavior(
                    KisConfig::Fixed);
                settings.setSelectionActionBarPosition(
                    KisConfig::Bottom);
            }
            KisConfigNotifier::instance()->notifyConfigChanged();
            menuBar()->setProperty("tabletWasVisible", menuBar()->isVisible());
            menuBar()->hide();
            if (QTabBar *tabs = d->findTabBarHACK()) {
                tabs->setProperty("tabletWasVisible", tabs->isVisible());
                tabs->hide();
            }
            if (statusBar()) {
                statusBar()->setProperty("tabletWasVisible", statusBar()->isVisible());
                statusBar()->hide();
            }
        } else {
            {
                KisConfig settings(false);
                settings.setSelectionActionBar(property("tabletPrevSelectionEnabled").toBool());
                settings.setSelectionActionBarBehavior(
                    static_cast<KisConfig::SelectionActionsBarBehavior>(
                        property("tabletPrevSelectionBehavior").toInt()));
                settings.setSelectionActionBarPosition(
                    static_cast<KisConfig::SelectionActionsBarPosition>(
                        property("tabletPrevSelectionPosition").toInt()));
            }
            KisConfigNotifier::instance()->notifyConfigChanged();
            menuBar()->setVisible(menuBar()->property("tabletWasVisible").toBool());
            if (QTabBar *tabs = d->findTabBarHACK()) {
                tabs->setVisible(tabs->property("tabletWasVisible").toBool());
            }
            if (statusBar()) {
                statusBar()->setVisible(statusBar()->property("tabletWasVisible").toBool());
            }
        }
    });
    connect(d->mdiArea, &QMdiArea::subWindowActivated, this,
            [this, tabletCompact](QMdiSubWindow *) {
        if (tabletCompact->isChecked()) {
            QTimer::singleShot(0, this, [this, tabletCompact] {
                if (!tabletCompact->isChecked()) return;
                if (QTabBar *tabs = d->findTabBarHACK()) tabs->hide();
            });
        }
    });
    QTimer::singleShot(0, this, [tabletCompact]() {
        tabletCompact->setChecked(QSettings().value(QStringLiteral("TabletUi/compact"), true).toBool());
    });

    QAction *oneShot = new QAction(i18n("Pick Color Once"), this);
    oneShot->setIcon(KisIconUtils::loadIcon("color-picker"));
    oneShot->setToolTip(i18n("Next stylus tap samples a color and returns to the brush"));
    actionCollection()->addAction(QStringLiteral("tablet_pick_once"), oneShot);
    connect(oneShot, &QAction::triggered, this, [this]() {
        QAction *picker = actionCollection()->action(QStringLiteral("KritaSelected/KisToolColorSampler"));
        if (!picker) return;
        QSettings().setValue(QStringLiteral("TabletUi/oneShotSampler"), true);
        picker->trigger();
    });

    QAction *sampleMerged = new QAction(i18n("Sample Visible Layers"), this);
    sampleMerged->setToolTip(i18n("Color picker samples all visible layers when checked, current layer otherwise"));
    sampleMerged->setIcon(KisIconUtils::loadIcon("color-picker"));
    sampleMerged->setCheckable(true);
    sampleMerged->setChecked(QSettings().value(QStringLiteral("TabletUi/sampleMerged"), true).toBool());
    actionCollection()->addAction(QStringLiteral("tablet_sample_merged"), sampleMerged);
    connect(sampleMerged, &QAction::toggled, this, [](bool checked) {
        QSettings().setValue(QStringLiteral("TabletUi/sampleMerged"), checked);
    });

    QAction *resetTabletView = new QAction(i18n("Reset Canvas View"), this);
    resetTabletView->setToolTip(i18n("Reset rotation and fit the entire image"));
    resetTabletView->setIcon(KisIconUtils::loadIcon("zoom-fit-best"));
    actionCollection()->addAction(QStringLiteral("tablet_reset_view"), resetTabletView);
    connect(resetTabletView, &QAction::triggered, this, [this]() {
        QAction *rotation = actionCollection()->action(QStringLiteral("reset_canvas_rotation"));
        QAction *fit = actionCollection()->action(QStringLiteral("zoom_to_fit"));
        if (rotation) rotation->trigger();
        if (fit) fit->trigger();
    });

    QAction *tabletSettings = new QAction(i18n("Tablet Controls..."), this);
    tabletSettings->setIcon(KisIconUtils::loadIcon("configure"));
    actionCollection()->addAction(QStringLiteral("tablet_settings"), tabletSettings);
    connect(tabletSettings, &QAction::triggered, this, [this]() {
        QSettings stored;
        QDialog dialog(this);
        dialog.setWindowFlags(Qt::Popup|Qt::FramelessWindowHint);
        dialog.setWindowTitle(i18n("Tablet Controls"));
        dialog.setStyleSheet(
            "QDialog{background:#29313b;color:#edf3f9;}"
            "QLabel{color:#eaf0f6;font-size:14px;}"
            "QCheckBox{color:#f4f7ff;min-height:52px;"
            "padding:8px;font-size:14px;spacing:12px;}"
            "QSpinBox{background:#3b4a5a;color:white;"
            "border:1px solid #7898ae;border-radius:8px;"
            "min-height:48px;padding:6px;font-size:15px;}"
            "QPushButton{background:#3d5366;color:white;"
            "border:1px solid #7193ac;border-radius:8px;"
            "min-height:50px;padding:5px;font-size:14px;}");
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(14,14,14,14);
        layout->setSpacing(10);
        QFormLayout *form = new QFormLayout;
        form->setRowWrapPolicy(QFormLayout::WrapAllRows);
        form->setVerticalSpacing(9);
        QSpinBox *doubleTapMs = new QSpinBox(&dialog);
        doubleTapMs->setRange(120, 600);
        doubleTapMs->setSuffix(i18n(" ms"));
        doubleTapMs->setValue(stored.value(QStringLiteral("TabletUi/doubleTapMs"), 250).toInt());
        form->addRow(i18n("Double-tap interval"), doubleTapMs);
        QSpinBox *pinchMs = new QSpinBox(&dialog);
        pinchMs->setRange(120, 500);
        pinchMs->setSuffix(i18n(" ms"));
        pinchMs->setValue(stored.value(QStringLiteral("TabletUi/pinchMs"), 250).toInt());
        form->addRow(i18n("Quick-pinch interval (experimental)"), pinchMs);
        QCheckBox *volumeKeys = new QCheckBox(i18n("Volume down: Undo / Volume up: Redo"), &dialog);
        volumeKeys->setChecked(stored.value(QStringLiteral("TabletUi/volumeKeys"), true).toBool());
        form->addRow(volumeKeys);
        QCheckBox *pinchEnabled = new QCheckBox(i18n("Enable quick-pinch reset (experimental)"), &dialog);
        pinchEnabled->setChecked(stored.value(QStringLiteral("TabletUi/quickPinch"), false).toBool());
        form->addRow(pinchEnabled);
        layout->addLayout(form);
        QDialogButtonBox *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        layout->addWidget(buttons);
        for(QAbstractButton *button:buttons->buttons())
            button->setMinimumHeight(50);
        dialog.resize(qMax(300,qMin(440,this->width()-30)),
                      qMax(360,qMin(640,this->height()-40)));
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() == QDialog::Accepted) {
            stored.setValue(QStringLiteral("TabletUi/doubleTapMs"), doubleTapMs->value());
            stored.setValue(QStringLiteral("TabletUi/pinchMs"), pinchMs->value());
            stored.setValue(QStringLiteral("TabletUi/volumeKeys"), volumeKeys->isChecked());
            stored.setValue(QStringLiteral("TabletUi/quickPinch"), pinchEnabled->isChecked());
        }
    });

    QMenu *quickMenu = new QMenu(i18n("Quick Actions"), this);
    quickMenu->menuAction()->setIcon(KisIconUtils::loadIcon("application-menu"));
    actionCollection()->addAction(QStringLiteral("tablet_quick_actions"), quickMenu->menuAction());
    connect(quickMenu, &QMenu::aboutToShow, this, [this, quickMenu]() {
        quickMenu->clear();
        auto add = [this, quickMenu](const char *name) {
            if (QAction *action = actionCollection()->action(QLatin1String(name))) {
                quickMenu->addAction(action);
            }
        };
        add("file_new"); add("file_open"); add("file_save_as");
        add("file_export_file");
        quickMenu->addSeparator();
        add("select_all"); add("invert_selection"); add("reselect");
        add("deselect"); add("fill_selection_foreground_color");
        quickMenu->addSeparator();
        add("duplicate_layer"); add("merge_layer"); add("quick_group");
        quickMenu->addSeparator();
        add("edit_undo"); add("edit_redo");
        add("tablet_reset_view");
        add("tablet_settings");
    });
#endif

    d->recentFiles