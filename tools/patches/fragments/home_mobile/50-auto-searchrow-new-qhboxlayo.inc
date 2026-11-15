        auto *searchRow=new QHBoxLayout;
        searchRow->setSpacing(6);
        auto *projectSearch=new QLineEdit(galleryPage);
        projectSearch->setPlaceholderText(i18n("Search projects"));
        projectSearch->setClearButtonEnabled(true);
        projectSearch->setMinimumHeight(48);
        auto *onlyStarred=new QToolButton(galleryPage);
        onlyStarred->setText(QStringLiteral("★"));
        onlyStarred->setAccessibleName(i18n("Show starred projects only"));
        onlyStarred->setToolTip(i18n("Show starred projects only"));
        onlyStarred->setCheckable(true);
        onlyStarred->setMinimumSize(52,48);
        onlyStarred->setStyleSheet(
            "QToolButton{background:#35383e;color:white;border:1px solid #777;"
            "border-radius:7px;font-size:21px;}"
            "QToolButton:checked{background:#6b613b;border:2px solid #e1c96c;}");
        searchRow->addWidget(projectSearch,1);
        searchRow->addWidget(onlyStarred);
        galleryLayout->insertLayout(1,searchRow);
        auto *projectCount=new QLabel(galleryPage);
        projectCount->setStyleSheet("color:#aeb7c5;font-size:12px;");
        galleryLayout->insertWidget(2,projectCount);
        const auto favorites=std::make_shared<QSet<QString>>();
        for(const QString &entry:QSettings().value(
                QStringLiteral("TabletUi/starredProjectsV1")).toStringList())
            favorites->insert(entry);
        auto filterProjects=[galleryTabs,galleryList,deviceList,projectSearch,
                             onlyStarred,projectCount,favorites]() {
            const QString query=projectSearch->text().trimmed();
            QListWidget *current=galleryTabs->currentIndex()==0 ?
                galleryList:deviceList;
            int matches=0,total=0;
            for(QListWidget *list:{galleryList,deviceList}) {
                for(int i=0;i<list->count();++i) {
                    QListWidgetItem *item=list->item(i);
                    const QString path=item->data(Qt::UserRole).toString();
                    QString name=item->data(Qt::UserRole+4).toString();
                    if(name.isEmpty()) {
                        name=item->text();
                        item->setData(Qt::UserRole+4,name);
                    }
                    const bool starred=favorites->contains(path);
                    const QString display=starred?QStringLiteral("★ ")+name:name;
                    if(item->text()!=display)item->setText(display);
                    const bool visible=(!onlyStarred->isChecked() || starred) &&
                        (query.isEmpty() ||
                         name.contains(query,Qt::CaseInsensitive) ||
                         path.contains(query,Qt::CaseInsensitive));
                    item->setHidden(!visible);
                    if(list==current) {
                        ++total;
                        if(visible)++matches;
                    }
                }
            }
            projectCount->setText(i18n("%1 of %2 projects",matches,total));
        };
        auto *filterTimer=new QTimer(galleryPage);
        filterTimer->setSingleShot(true);
        const auto scheduleFilter=[filterTimer]() {
            if(!filterTimer->isActive())filterTimer->start(0);
        };
        connect(filterTimer,&QTimer::timeout,this,filterProjects);
        connect(projectSearch,&QLineEdit::textChanged,this,
                [scheduleFilter](const QString&) {scheduleFilter();});
        connect(onlyStarred,&QToolButton::toggled,this,
                [scheduleFilter](bool) {scheduleFilter();});
        connect(galleryTabs,&QTabWidget::currentChanged,this,
                [scheduleFilter](int) {scheduleFilter();});
        for(QListWidget *list:{galleryList,deviceList}) {
            connect(list->model(),&QAbstractItemModel::rowsInserted,this,
                    [scheduleFilter](const QModelIndex&,int,int) {
                scheduleFilter();
            });
            connect(list->model(),&QAbstractItemModel::modelReset,this,
                    scheduleFilter);
        }
        scheduleFilter();
