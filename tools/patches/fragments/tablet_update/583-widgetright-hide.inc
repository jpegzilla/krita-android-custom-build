        widgetRight->hide();
        clearRecentFilesLink->hide();
        clearRecentFilesLink->setEnabled(false);
        auto *galleryButton = new QToolButton(this);
        galleryButton->setText(i18n("Project Gallery"));
        galleryButton->setToolTip(i18n("Switch between recent projects and the folder gallery"));
        horizontalLayout_10->addWidget(galleryButton);
        QStackedWidget *pages = recentDocsStackedWidget;
        QWidget *galleryPage = new QWidget(pages);
        QVBoxLayout *galleryLayout = new QVBoxLayout(galleryPage);
        auto *galleryControls = new QHBoxLayout;
        auto *folderButton = new QPushButton(i18n("Choose Folder"), galleryPage);
        auto *refreshButton = new QPushButton(i18n("Refresh"), galleryPage);
        auto *sortMode = new QComboBox(galleryPage);
        sortMode->addItem(i18n("Newest first"));
        sortMode->addItem(i18n("Name A-Z"));
        galleryControls->addWidget(folderButton);
        galleryControls->addWidget(refreshButton);
        galleryControls->addWidget(sortMode);
        galleryLayout->addLayout(galleryControls);
        auto *galleryList = new QListWidget(galleryPage);
        galleryList->setViewMode(QListView::IconMode);
        galleryList->setIconSize(QSize(156, 156));
        galleryList->setGridSize(QSize(184, 205));
        galleryList->setResizeMode(QListView::Adjust);
        galleryList->setMovement(QListView::Static);
        galleryList->setWordWrap(true);
        galleryLayout->addWidget(galleryList);
        pages->addWidget(galleryPage);
        auto folder = std::make_shared<QString>(
            QSettings().value(QStringLiteral("CustomProjectGalleryFolder"), QDir::homePath()).toString());
        auto populate = [this, galleryList, folder, sortMode]() {
            galleryList->clear();
            if (!folder->isEmpty() && QDir(*folder).exists()) {
                QFileInfoList files;
                QDirIterator iter(*folder, QStringList() << QStringLiteral("*.kra")
                                                        << QStringLiteral("*.krz"),
                                  QDir::Files, QDirIterator::Subdirectories);
                while (iter.hasNext() && files.size() < 2000) {
                    iter.next();
                    files.append(iter.fileInfo());
                }
                if (sortMode->currentIndex() == 0) {
                    std::sort(files.begin(), files.end(), [](const QFileInfo &a, const QFileInfo &b) {
                        return a.lastModified() > b.lastModified();
                    });
                } else {
                    std::sort(files.begin(), files.end(), [](const QFileInfo &a, const QFileInfo &b) {
                        return a.fileName().compare(b.fileName(), Qt::CaseInsensitive) < 0;
                    });
                }
                for (const QFileInfo &file : files) {
                    QIcon icon;
                    QScopedPointer<KoStore> store(KoStore::createStore(file.absoluteFilePath(), KoStore::Read));
                    if (store && !store->bad()) {
                        for (const QString &entry : {QStringLiteral("preview.png"),
                                                     QStringLiteral("Thumbnails/thumbnail.png")}) {
                            if (!store->hasFile(entry) || !store->open(entry)) continue;
                            QImage image;
                            image.loadFromData(store->read(store->size()), "PNG");
                            store->close();
                            if (!image.isNull()) {
                                icon = QIcon(QPixmap::fromImage(image.scaled(
                                    156, 156, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
                                break;
                            }
                        }
                    }
                    auto *item = new QListWidgetItem(icon, file.completeBaseName(), galleryList);
                    item->setData(Qt::UserRole, file.absoluteFilePath());
                }
            }
            // Keep SAF content URI projects in reach through Krita recent files.
            QSet<QString> seen;
            for (int i = 0; i < galleryList->count(); ++i)
                seen.insert(galleryList->item(i)->data(Qt::UserRole).toString());
            const QAbstractItemModel *recent = recentDocumentsListView->model();
            if (recent) {
                for (int i = 0; i < recent->rowCount(); ++i) {
                    QModelIndex index = recent->index(i, 0);
                    QString uri = index.data(Qt::ToolTipRole).toString();
                    if (uri.isEmpty() || seen.contains(uri)) continue;
                    auto *item = new QListWidgetItem(
                        qvariant_cast<QIcon>(index.data(Qt::DecorationRole)),
                        index.data(Qt::DisplayRole).toString(), galleryList);
                    item->setData(Qt::UserRole, uri);
                    seen.insert(uri);
                }
            }
        };
        connect(galleryButton, &QToolButton::clicked, this,
                [pages, galleryPage, galleryButton, populate, this]() {
            if (pages->currentWidget() == galleryPage) {
                // Krita's recent-file model loads asynchronously and may
                // replace the initially saved placeholder with the real list.
                // Always restore based on the live model, not recentPage.
                const bool hasRecent=recentDocumentsListView->model() &&
                    recentDocumentsListView->model()->rowCount()>0;
                pages->setCurrentWidget(hasRecent ?
                    static_cast<QWidget*>(recentDocumentsListView) :
                    static_cast<QWidget*>(labelNoRecentDocs));
                galleryButton->setText(i18n("Project Gallery"));
                recentDocumentsLabel->setText(i18n("Recent Images"));
                clearRecentFilesLink->hide();
            } else {
                populate();
                pages->setCurrentWidget(galleryPage);
                galleryButton->setText(i18n("Recent Images"));
                recentDocumentsLabel->setText(i18n("Project Gallery"));
                clearRecentFilesLink->hide();
            }
        });
        connect(folderButton, &QPushButton::clicked, this, [this, folder, populate]() {
            QString selection = QFileDialog::getExistingDirectory(
                this, i18n("Choose a Krita Projects Folder"), *folder);
            if (!selection.isEmpty()) {
                *folder = selection;
                QSettings().setValue(QStringLiteral("CustomProjectGalleryFolder"), *folder);
                populate();
            }
        });
        connect(refreshButton, &QPushButton::clicked, this, populate);
        connect(sortMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [populate](int) { populate(); });
        connect(galleryList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
            m_mainWindow->openDocument(item->data(Qt::UserRole).toString(), KisMainWindow::None);
        });
