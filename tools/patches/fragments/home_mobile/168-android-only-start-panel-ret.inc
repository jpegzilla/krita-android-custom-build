        // Android-only start panel: retain native Krita's document
        // creation/open actions and recent file model.
        widgetCenter->hide();
        widgetRight->hide();
        horizontalLayout_16->setDirection(QBoxLayout::TopToBottom);
        horizontalLayout_16->setContentsMargins(5,5,5,5);
        horizontalLayout_16->setSpacing(8);
        verticalLayout_3->setContentsMargins(7,7,7,7);
        widgetRecentImagesColumn->setMinimumWidth(0);
        widgetRecentImagesColumn->setSizePolicy(
            QSizePolicy::Expanding,QSizePolicy::Expanding);
        auto *homePanel=new QFrame(dropFrameBorder);
        homePanel->setObjectName(QStringLiteral("tabletHomePanel"));
        homePanel->setStyleSheet(
            "QFrame#tabletHomePanel{background:#292c33;"
            "border:1px solid #434956;border-radius:13px;}"
            "QLabel#tabletHomeTitle{color:#fafafa;font-size:26px;font-weight:600;}"
            "QLabel#tabletHomeSubtitle{color:#bbc3d1;font-size:12px;}"
            "QToolButton#tabletHomeAction{background:#39404a;color:#fafafa;"
            "border:1px solid #667181;border-radius:9px;"
            "padding:6px;font-size:13px;font-weight:600;}"
            "QToolButton#tabletHomePrimary{background:#476a85;color:white;"
            "border:1px solid #719cb9;border-radius:9px;"
            "padding:6px;font-size:13px;font-weight:600;}"
            "QToolButton#tabletHomeContinue{background:#343943;color:white;"
            "border:1px solid #606b7c;border-radius:9px;padding:8px;"
            "font-size:12px;text-align:left;}");
        auto *homeLayout=new QVBoxLayout(homePanel);
        homeLayout->setContentsMargins(12,10,12,10);
        homeLayout->setSpacing(8);
        auto *title=new QLabel(i18n("Krita"),homePanel);
        title->setObjectName(QStringLiteral("tabletHomeTitle"));
        homeLayout->addWidget(title);
        auto *subtitle=new QLabel(
            i18n("Start drawing or pick up where you left off."),homePanel);
        subtitle->setObjectName(QStringLiteral("tabletHomeSubtitle"));
        subtitle->setWordWrap(true);
        homeLayout->addWidget(subtitle);
        auto *actions=new QGridLayout;
        actions->setSpacing(8);
        auto makeAction=[homePanel](const QString &text,
                                     const QString &icon,bool primary) {
            auto *button=new QToolButton(homePanel);
            button->setObjectName(primary?
                QStringLiteral("tabletHomePrimary"):
                QStringLiteral("tabletHomeAction"));
            button->setText(text);
            button->setIcon(KisIconUtils::loadIcon(icon));
            button->setIconSize(QSize(24,24));
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            button->setMinimumHeight(60);
            button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
            return button;
        };
        auto *newCanvas=makeAction(
            i18n("New canvas"),QStringLiteral("document-new"),true);
        auto *openFile=makeAction(
            i18n("Open file"),QStringLiteral("document-open"),false);
        actions->addWidget(newCanvas,0,0);
        actions->addWidget(openFile,0,1);
        actions->setColumnStretch(0,1);
        actions->setColumnStretch(1,1);
        connect(newCanvas,&QToolButton::clicked,this,
                [this]() {slotNewFileClicked();});
        connect(openFile,&QToolButton::clicked,this,
                [this]() {slotOpenFileClicked();});
        horizontalLayout_10->removeWidget(galleryButton);
        galleryButton->setParent(homePanel);
        galleryButton->setObjectName(QStringLiteral("tabletHomeAction"));
        galleryButton->setIcon(KisIconUtils::loadIcon("folder-pictures"));
        galleryButton->setIconSize(QSize(24,24));
        galleryButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        galleryButton->setMinimumHeight(58);
        galleryButton->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        actions->addWidget(galleryButton,1,0,1,2);
        homeLayout->addLayout(actions);
        auto *continueButton=new QToolButton(homePanel);
        continueButton->setObjectName(QStringLiteral("tabletHomeContinue"));
        continueButton->setText(i18n("Continue last project"));
        continueButton->setIcon(KisIconUtils::loadIcon("document-open-recent"));
        continueButton->setIconSize(QSize(23,23));
        continueButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        continueButton->setMinimumHeight(48);
        continueButton->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        homeLayout->addWidget(continueButton);
        const auto updateContinue=[this,continueButton]() {
            const QAbstractItemModel *model=recentDocumentsListView->model();
            const bool available=model && model->rowCount()>0;
            continueButton->setVisible(available);
            if(!available)return;
            QString name=model->index(0,0).data(Qt::DisplayRole).toString();
            if(name.isEmpty())name=i18n("Last project");
            name=QFontMetrics(continueButton->font()).elidedText(
                name,Qt::ElideMiddle,qMax(100,continueButton->width()-80));
            continueButton->setText(i18n("Continue: %1",name));
        };
        connect(continueButton,&QToolButton::clicked,this,[this]() {
            const QAbstractItemModel *model=recentDocumentsListView->model();
            if(model && model->rowCount()>0)
                recentDocumentClicked(model->index(0,0));
        });
        QTimer::singleShot(0,this,[this,updateContinue]() {
            updateContinue();
            if(QAbstractItemModel *model=recentDocumentsListView->model()) {
                connect(model,&QAbstractItemModel::modelReset,this,
                        updateContinue);
                connect(model,&QAbstractItemModel::rowsInserted,this,
                        [updateContinue](const QModelIndex&,int,int) {
                    updateContinue();
                });
                connect(model,&QAbstractItemModel::rowsRemoved,this,
                        [updateContinue](const QModelIndex&,int,int) {
                    updateContinue();
                });
            }
        });
        horizontalLayout_16->insertWidget(0,homePanel);
        recentDocumentsLabel->setText(i18n("Recent work"));
        recentDocumentsListView->setIconSize(QSize(110,110));
        recentDocumentsListView->setGridSize(QSize(156,178));
        recentDocumentsListView->setSpacing(7);
        recentDocumentsListView->setResizeMode(QListView::Adjust);
        recentDocumentsListView->setMovement(QListView::Static);
        recentDocumentsListView->setWrapping(true);
        recentDocumentsListView->setWordWrap(true);
        recentDocumentsListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        recentDocsStackedWidget->setMinimumHeight(200);
