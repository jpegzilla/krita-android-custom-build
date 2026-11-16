#ifdef Q_OS_ANDROID
    if (parent && parent->property("_mobileBrushEditor").toBool()) {
        auto &ui = m_d->uiWdgPaintOpPresetSettings;
        setObjectName(QStringLiteral("mobileAdvancedBrushEditor"));
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet(QStringLiteral(
            "QWidget#mobileAdvancedBrushEditor,QStackedWidget,QScrollArea,QListView{"
            "background:#25262b;color:#f1f2f5;border:0;}"
            "QLabel,QCheckBox,QRadioButton{color:#f1f2f5;}"
            "QPushButton,QToolButton,QComboBox{background:#30323b;color:#f1f2f5;"
            "border:1px solid #44464f;border-radius:7px;padding:3px 7px;}"
            "QPushButton:pressed,QToolButton:checked,QListView::item:selected{background:#394459;}"
            "QPushButton:disabled,QToolButton:disabled{color:#989da9;}"));
        ui.gridLayout->setContentsMargins(10, 8, 10, 8);
        ui.gridLayout->setSpacing(6);
        ui.brushEditorDashboard->setDirection(QBoxLayout::TopToBottom);
        ui.brushEditorDashboard->setSpacing(3);
        ui.presetThumbnailicon->hide();
        ui.liveBrushPreviewView->setMinimumWidth(0);
        ui.liveBrushPreviewView->setMaximumWidth(QWIDGETSIZE_MAX);
        ui.liveBrushPreviewView->setFixedHeight(42);
        ui.liveBrushPreviewView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        ui.currentBrushNameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        ui.currentBrushNameLabel->setWordWrap(true);
        ui.currentBrushEngineLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        ui.verticalLayout_5->setDirection(QBoxLayout::LeftToRight);
        for (auto *button : {ui.saveNewBrushPresetButton, ui.saveBrushPresetButton}) {
            button->setMinimumHeight(38);
            button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        }
        ui.showPresetsButton->hide();
        ui.showScratchpadButton->hide();
        ui.viewOptionButton->hide();

        auto *behavior = new QWidget(this);
        auto *behaviorLayout = new QVBoxLayout(behavior);
        ui.brushEditorLayout->removeItem(ui.horizontalLayout_3);
        ui.horizontalLayout_3->setParent(nullptr);
        ui.horizontalLayout_3->setDirection(QBoxLayout::TopToBottom);
        behaviorLayout->addLayout(ui.horizontalLayout_3);
        behaviorLayout->addStretch(1);

        m_d->mobileTabs = new QTabBar(this);
        m_d->mobileTabs->setExpanding(true);
        m_d->mobileTabs->setElideMode(Qt::ElideRight);
        m_d->mobileTabs->setStyleSheet(QStringLiteral(
            "QTabBar::tab{background:#30323b;color:#bfc3ce;min-height:40px;padding:0 8px;}"
            "QTabBar::tab:selected{background:#394459;color:#f1f2f5;}"
            "QTabBar::tab:disabled{color:#989da9;}"));
        auto *pages = new QStackedWidget(this);
        const auto addPage = [this, pages](QWidget *page, const QString &label) {
            page->setMinimumSize(0, 0);
            page->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            pages->addWidget(page);
            m_d->mobileTabs->addTab(label);
        };
        addPage(ui.brushEditorSettingsControls, i18n("Brush"));
        addPage(ui.scratchpadControls, i18n("Test"));
        addPage(ui.presetsContainer, i18n("Presets"));
        auto *behaviorScroll = new QScrollArea(this);
        behaviorScroll->setFrameShape(QFrame::NoFrame);
        behaviorScroll->setWidgetResizable(true);
        behaviorScroll->setWidget(behavior);
        MobileTouchScroll::configure(behaviorScroll);
        addPage(behaviorScroll, i18n("Behavior"));
        MobileTouchScroll::configureList(ui.presetWidget->itemChooser()->itemView());
        connect(m_d->mobileTabs, &QTabBar::currentChanged, pages, &QStackedWidget::setCurrentIndex);
        auto *done = new QPushButton(i18n("Done"), this);
        done->setMinimumHeight(42);
        done->setStyleSheet(QStringLiteral("QPushButton{background:#394459;font-weight:600;}"));
        connect(done, &QPushButton::clicked, this, [this]() {
            if (parentWidget()) parentWidget()->hide();
        });
        ui.gridLayout->addWidget(m_d->mobileTabs, 1, 0, 1, 3);
        ui.gridLayout->addWidget(pages, 2, 0, 1, 3);
        ui.gridLayout->addWidget(done, 3, 0, 1, 3);
        ui.gridLayout->setRowStretch(2, 1);
        return;
    }
#endif

    // Responsive Layout
