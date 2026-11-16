        connect(openFileLink, SIGNAL(clicked(bool)), this, SLOT(slotOpenFileClicked()));
#ifdef Q_OS_ANDROID
        auto *galleryButton = new QToolButton(this);
        galleryButton->setText(i18n("Project Gallery"));
        horizontalLayout_2->addWidget(galleryButton);
        connect(galleryButton, &QToolButton::clicked, this, [this]() {
            QDialog gallery(this);
            gallery.setWindowTitle(i18n("Project Gallery"));
            gallery.resize(760, 640);
            auto *column = new QVBoxLayout(&gallery);
            auto *buttons = new QHBoxLayout;
            auto *chooseFolder = new QPushButton(i18n("Choose Folder"), &gallery);
            auto *refresh = new QPushButton(i18n("Refresh"), &gallery);
            auto *newCanvas = new QPushButton(i18n("New Canvas"), &gallery);
            buttons->addWidget(chooseFolder);
            buttons->addWidget(refresh);
            buttons->addStretch(1);
            buttons->addWidget(newCanvas);
            column->addLayout(buttons);
            auto *projectList = new QListWidget(&gallery);
            projectList->setViewMode(QListView::IconMode);
            projectList->setIconSize(QSize(156, 156));
            projectList->setGridSize(QSize(184, 205));
            projectList->setResizeMode(QListView::Adjust);
            projectList->setMovement(QListView::Static);
            projectList->setWordWrap(true);
            projectList->setSpacing(5);
            column->addWidget(projectList);
            QSettings settings;
            QString folder = settings.value(QStringLiteral("CustomProjectGalleryFolder"),
                                            QDir::homePath()).toString();
            auto populate = [&]() {
                projectList->clear();
                if (!folder.isEmpty() && QDir(folder).exists()) {
                    QFileInfoList projects;
                    QDirIterator iter(folder, QStringList() << QStringLiteral("*.kra")
                                                            << QStringLiteral("*.krz"),
                                      QDir::Files, QDirIterator::Subdirectories);
                    while (iter.hasNext() && projects.size() < 2000) {
                        iter.next();
                        projects.append(iter.fileInfo());
                    }
                    std::sort(projects.begin(), projects.end(),
                              [](const QFileInfo &a, const QFileInfo &b) {
                                  return a.lastModified() > b.lastModified();
                              });
                    for (const QFileInfo &file : projects) {
                        QIcon thumbnail;
                        QScopedPointer<KoStore> store(KoStore::createStore(file.absoluteFilePath(), KoStore::Read));
                        if (store && !store->bad()) {
                            for (const QString &entry : {QStringLiteral("preview.png"),
                                                         QStringLiteral("Thumbnails/thumbnail.png")}) {
                                if (!store->hasFile(entry) || !store->open(entry)) continue;
                                QImage image;
                                image.loadFromData(store->read(store->size()), "PNG");
                                store->close();
                                if (!image.isNull()) {
                                    thumbnail = QIcon(QPixmap::fromImage(image.scaled(
                                        156, 156, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
                                    break;
                                }
                            }
                        }
                        auto *item = new QListWidgetItem(thumbnail, file.completeBaseName(), projectList);
                        item->setData(Qt::UserRole, file.absoluteFilePath());
                    }
                }
                // Include Krita's recent documents, including content:// URIs,
                // which normal filesystem scanning cannot enumerate.
                QSet<QString> seen;
                for (int i = 0; i < projectList->count(); ++i) {
                    seen.insert(projectList->item(i)->data(Qt::UserRole).toString());
                }
                const QAbstractItemModel *recent = recentDocumentsListView->model();
                if (recent) {
                    for (int row = 0; row < recent->rowCount(); ++row) {
                        QModelIndex index = recent->index(row, 0);
                        QString uri = index.data(Qt::ToolTipRole).toString();
                        if (uri.isEmpty() || seen.contains(uri)) continue;
                        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
                        auto *item = new QListWidgetItem(icon, index.data(Qt::DisplayRole).toString(), projectList);
                        item->setData(Qt::UserRole, uri);
                        seen.insert(uri);
                    }
                }
            };
            connect(chooseFolder, &QPushButton::clicked, &gallery, [&]() {
                QString path = QFileDialog::getExistingDirectory(
                    &gallery, i18n("Choose a Krita Projects Folder"), folder);
                if (!path.isEmpty()) {
                    folder = path;
                    settings.setValue(QStringLiteral("CustomProjectGalleryFolder"), folder);
                    populate();
                }
            });
            connect(refresh, &QPushButton::clicked, &gallery, populate);
            connect(newCanvas, &QPushButton::clicked, &gallery, [&]() {
                gallery.accept();
                slotNewFileClicked();
            });
            connect(projectList, &QListWidget::itemClicked, &gallery,
                    [&](QListWidgetItem *item) {
                        if (m_mainWindow->openDocument(item->data(Qt::UserRole).toString(), KisMainWindow::None)) {
                            gallery.accept();
                        }
                    });
            populate();
            gallery.exec();
        });
#endif
