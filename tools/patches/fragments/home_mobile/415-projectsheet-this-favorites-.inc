        *projectSheet=[this,favorites,scheduleFilter,
                       moveToGalleryTrash](QListWidgetItem *item) {
            if(!item)return;
            const QString path=item->data(Qt::UserRole).toString();
            QString name=item->data(Qt::UserRole+4).toString();
            if(name.isEmpty())name=item->text();
            if(name.startsWith(QStringLiteral("★ ")))name=name.mid(2);
            const QFileInfo info(path);
            const bool local=info.isFile();
            QDialog sheet(this);
            // Qt::Popup dismisses on outside tap, like a mobile bottom sheet.
            // The underlying project remains unopened until Open is pressed.
            sheet.setWindowFlags(Qt::Popup|Qt::FramelessWindowHint);
            sheet.setWindowTitle(name);
            sheet.resize(qMax(300,qMin(410,width()-30)),
                         qMax(390,qMin(600,height()-40)));
            sheet.setStyleSheet(
                "QDialog{background:#2b3039;color:#f5f6f8;}"
                "QLabel{color:#e0e6ee;}"
                "QPushButton{background:#404956;color:white;"
                "border:1px solid #66778a;border-radius:9px;"
                "padding:8px;font-size:14px;}"
                "QPushButton:pressed{background:#557e98;}"
                "QPushButton#projectOpen{background:#476f8c;"
                "border:2px solid #8eb8d2;font-size:17px;font-weight:600;}");
            auto *layout=new QVBoxLayout(&sheet);
            layout->setContentsMargins(16,16,16,16);
            layout->setSpacing(9);
            auto *preview=new QLabel(&sheet);
            preview->setAlignment(Qt::AlignCenter);
            if(!item->icon().isNull()) {
                preview->setPixmap(item->icon().pixmap(QSize(130,130)));
                layout->addWidget(preview);
            }
            auto *heading=new QLabel(name,&sheet);
            heading->setWordWrap(true);
            heading->setAlignment(Qt::AlignCenter);
            heading->setStyleSheet("font-size:17px;font-weight:600;");
            layout->addWidget(heading);
            auto *description=new QLabel(&sheet);
            description->setAlignment(Qt::AlignCenter);
            description->setWordWrap(true);
            const QString ext=QFileInfo(name).suffix().toUpper();
            if(local) {
                description->setText(i18n("%1 · %2 KB · Modified %3",
                    ext,QString::number((info.size()+1023)/1024),
                    info.lastModified().toString(QStringLiteral("MMM d, yyyy"))));
            } else {
                description->setText(i18n("%1 project",ext));
            }
            layout->addWidget(description);
            auto *open=new QPushButton(i18n("Open project"),&sheet);
            open->setObjectName(QStringLiteral("projectOpen"));
            open->setIcon(KisIconUtils::loadIcon("document-open"));
            open->setMinimumHeight(66);
            layout->addWidget(open);
            connect(open,&QPushButton::clicked,&sheet,
                    [this,path,&sheet]() {
                sheet.accept();
                m_mainWindow->openDocument(path,KisMainWindow::None);
            });
            auto *star=new QPushButton(&sheet);
            star->setMinimumHeight(54);
            auto refreshStar=[star,favorites,path]() {
                star->setText(favorites->contains(path)?
                    i18n("★ Remove star"):i18n("☆ Star project"));
            };
            refreshStar();
            layout->addWidget(star);
            connect(star,&QPushButton::clicked,&sheet,
                    [favorites,path,scheduleFilter,refreshStar]() {
                if(favorites->contains(path))favorites->remove(path);
                else favorites->insert(path);
                QStringList saved;
                for(const QString &entry:*favorites)saved.append(entry);
                QSettings().setValue(QStringLiteral("TabletUi/starredProjectsV1"),
                                     saved);
                scheduleFilter();
                refreshStar();
            });
            auto *copy=new QPushButton(i18n("Copy file location"),&sheet);
            copy->setMinimumHeight(52);
            layout->addWidget(copy);
            connect(copy,&QPushButton::clicked,&sheet,[path,&sheet]() {
                QGuiApplication::clipboard()->setText(path);
                sheet.accept();
            });
            if(local) {
                auto *trash=new QPushButton(i18n("Move to Trash (undoable)"),&sheet);
                trash->setMinimumHeight(52);
                layout->addWidget(trash);
                connect(trash,&QPushButton::clicked,&sheet,
                        [path,moveToGalleryTrash,&sheet]() {
                    sheet.accept();
                    moveToGalleryTrash(path);
                });
            }
            auto *close=new QPushButton(i18n("Close"),&sheet);
            close->setMinimumHeight(48);
            layout->addWidget(close);
            connect(close,&QPushButton::clicked,&sheet,&QDialog::reject);
            sheet.exec();
        };
