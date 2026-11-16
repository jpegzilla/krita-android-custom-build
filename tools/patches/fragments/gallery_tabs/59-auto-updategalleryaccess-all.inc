        auto updateGalleryAccess=[allowFiles]() {
            const bool granted=QAndroidJniObject::callStaticMethod<jboolean>(
                "org/krita/android/MainActivity","tabletHasAllFilesAccess","()Z");
            allowFiles->setVisible(!granted);
        };
        updateGalleryAccess();
        QObject::connect(qApp,&QGuiApplication::applicationStateChanged,this,
                         [updateGalleryAccess](Qt::ApplicationState state) {
            if(state==Qt::ApplicationActive)updateGalleryAccess();
        });

        const auto acceptedExtensions=QStringList{
            QStringLiteral("*.kra*"),QStringLiteral("*.krz*")
        };
        const QRegularExpression validKritaProject(
            QStringLiteral("(?i)\\.(?:kra|krz)(?: \\([0-9]+\\))?$"));
        const QStringList scanRoots={
            QStringLiteral("/storage/emulated/0/Download"),
            QStringLiteral("/storage/emulated/0/Documents"),
            QStringLiteral("/storage/emulated/0/Pictures/Krita"),
            QStringLiteral("/storage/emulated/0/Krita")
        };
        auto deviceSeen=std::make_shared<QSet<QString>>();
        auto deviceIterator=std::make_shared<std::shared_ptr<QDirIterator>>();
        auto deviceCount=std::make_shared<int>(0);
        auto deviceRootIndex=std::make_shared<int>(0);
        auto deviceScanRoots=std::make_shared<QStringList>(scanRoots);
        auto deviceThumbBudget=std::make_shared<int>(48);
        auto scanTimer=new QTimer(galleryPage);
        scanTimer->setInterval(10);
        auto addDeviceItem=[deviceList,deviceSeen,deviceThumbBudget,
                            validKritaProject](const QString &location,
                                              const QString &title,
                                              const QString &physicalPath) {
            if(location.isEmpty() || deviceSeen->contains(location))return;
            if(!validKritaProject.match(QFileInfo(
                    physicalPath.isEmpty()?title:physicalPath).fileName()).hasMatch())
                return;
            if(!physicalPath.isEmpty() && deviceSeen->contains(physicalPath))return;
            QIcon thumbnail;
            if(*deviceThumbBudget>0 && !physicalPath.isEmpty() &&
               QFileInfo(physicalPath).isReadable() &&
               QFileInfo(physicalPath).size()<134217728) {
                --*deviceThumbBudget;
                QScopedPointer<KoStore> store(KoStore::createStore(physicalPath,KoStore::Read));
                if(store && !store->bad()) {
                    for(const QString &preview:{QStringLiteral("preview.png"),
                                                 QStringLiteral("Thumbnails/thumbnail.png")}) {
                        if(!store->hasFile(preview) || !store->open(preview))continue;
                        QImage image;
                        image.loadFromData(store->read(store->size()),"PNG");
                        store->close();
                        if(!image.isNull()) {
                            thumbnail=QIcon(QPixmap::fromImage(image.scaled(
                                156,156,Qt::KeepAspectRatio,Qt::SmoothTransformation)));
                            break;
                        }
                    }
                }
            }
            auto *item=new QListWidgetItem(thumbnail,title,deviceList);
            item->setData(Qt::UserRole,location);
            item->setToolTip(physicalPath.isEmpty()?location:physicalPath);
            deviceSeen->insert(location);
            if(!physicalPath.isEmpty())deviceSeen->insert(physicalPath);
        };
        QObject::connect(scanTimer,&QTimer::timeout,this,
            [scanTimer,deviceIterator,deviceCount,deviceRootIndex,
             deviceScanRoots,addDeviceItem,validKritaProject]() {
            int batch=0;
            while(batch++<12 && *deviceCount<200) {
                if(!*deviceIterator || !(*deviceIterator)->hasNext()) {
                    deviceIterator->reset();
                    while(*deviceRootIndex<deviceScanRoots->size() &&
                          !QDir(deviceScanRoots->at(*deviceRootIndex)).exists())
                        ++*deviceRootIndex;
                    if(*deviceRootIndex>=deviceScanRoots->size()){
                        scanTimer->stop();break;
                    }
                    const QString root=deviceScanRoots->at((*deviceRootIndex)++);
                    *deviceIterator=std::make_shared<QDirIterator>(
                        root,QStringList{QStringLiteral("*.kra*"),
                                         QStringLiteral("*.krz*")},
                        QDir::Files|QDir::Readable|QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
                }
                if(!(*deviceIterator)->hasNext())continue;
                const QString path=(*deviceIterator)->next();
                const QFileInfo info(path);
                if(!info.isReadable() ||
                   !validKritaProject.match(info.fileName()).hasMatch() ||
                   info.absolutePath().contains(
                       QStringLiteral(".krita-gallery-trash")))continue;
                addDeviceItem(info.absoluteFilePath(),info.fileName(),
                              info.absoluteFilePath());
                ++*deviceCount;
            }
            if(*deviceCount>=200) {
                scanTimer->stop();deviceIterator->reset();
            }
        });
        auto populateDevice=[this,deviceList,deviceSeen,deviceIterator,deviceCount,
                             deviceRootIndex,deviceScanRoots,deviceThumbBudget,
                             scanTimer,acceptedExtensions,addDeviceItem,
                             updateGalleryAccess,folder]() {
            scanTimer->stop();
            deviceIterator->reset();
            deviceList->clear();
            deviceSeen->clear();
            *deviceCount=0;
            *deviceRootIndex=0;
            *deviceThumbBudget=48;
            *deviceScanRoots=QStringList{*folder,
                QStringLiteral("/storage/emulated/0/Download"),
                QStringLiteral("/storage/emulated/0/Documents"),
                QStringLiteral("/storage/emulated/0/Pictures/Krita"),
                QStringLiteral("/storage/emulated/0/Krita")};
            deviceScanRoots->removeDuplicates();
            updateGalleryAccess();
            // Avoid a synchronous whole-device MediaStore query; it used to
            // delay the welcome screen and mix photos into the project list.
            // The selected folder and four likely save folders are scanned in
            // short timer batches only when this tab is opened.
            scanTimer->start();
        };
        connect(galleryTabs,&QTabWidget::currentChanged,this,
                [galleryTabs,populateDevice,scanTimer](int index) {
            if(index==1)populateDevice();
            else scanTimer->stop();
        });
        auto suppressGalleryClick=std::make_shared<QSet<QListWidget*>>();
        connect(deviceList,&QListWidget::itemClicked,this,
                [this,deviceList,suppressGalleryClick](QListWidgetItem *item) {
            if(suppressGalleryClick->remove(deviceList))return;
            m_mainWindow->openDocument(item->data(Qt::UserRole).toString(),
                                        KisMainWindow::None);
        });
        // Trash never permanently deletes a project. It renames into an
        // adjacent hidden folder on the same filesystem and saves a stack
        // of original paths for reversible restoration, even after restart.
        auto undoTrash=new QPushButton(i18n("Undo Trash"),galleryPage);
        galleryControls->addWidget(undoTrash);
        auto trashStack=[]() {
            return QSettings().value(
                QStringLiteral("TabletUi/galleryTrashStack")).toList();
        };
        auto syncUndo=[undoTrash,trashStack]() {
            const QVariantList stack=trashStack();
            undoTrash->setEnabled(!stack.isEmpty() &&
                QFileInfo(stack.last().toMap().value(
                    QStringLiteral("trash")).toString()).isFile());
        };
        syncUndo();
        auto refreshGallery=[galleryTabs,populate,populateDevice]() {
            if(galleryTabs->currentIndex()==0)populate();
            else populateDevice();
        };
        connect(undoTrash,&QPushButton::clicked,this,
                [this,trashStack,syncUndo,refreshGallery]() {
            QVariantList stack=trashStack();
            if(stack.isEmpty())return;
            const QVariantMap item=stack.last().toMap();
            const QString from=item.value(QStringLiteral("trash")).toString();
            const QString to=item.value(QStringLiteral("original")).toString();
            if(!QFileInfo(from).isFile() || QFileInfo::exists(to)) {
                QMessageBox::warning(this,i18n("Undo Trash"),
                    i18n("Cannot restore: the trash file is missing or the original name is occupied."));
                return;
            }
            if(!QFile::rename(from,to)) {
                QMessageBox::warning(this,i18n("Undo Trash"),
                    i18n("Restore failed. The project remains in the trash folder."));
                return;
            }
            stack.removeLast();
            QSettings().setValue(QStringLiteral("TabletUi/galleryTrashStack"),stack);
            syncUndo();
            refreshGallery();
        });
        auto moveToGalleryTrash=[this,trashStack,syncUndo,refreshGallery](
                                  const QString &source) {
            if(source.startsWith(QStringLiteral("content://")) ||
               source.startsWith(QStringLiteral("file://"))) {
                QMessageBox::information(this,i18n("Project Trash"),
                    i18n("Android's document-provider item cannot be moved safely here. Use its file manager."));
                return;
            }
            const QFileInfo file(source);
            if(!file.isFile() || !file.isWritable() ||
               !QRegularExpression(QStringLiteral(
                   "(?i)\\.(?:kra|krz)(?: \\([0-9]+\\))?$")).match(file.fileName()).hasMatch()) {
                QMessageBox::information(this,i18n("Project Trash"),
                    i18n("Only writable local Krita project files can be moved to Trash."));
                return;
            }
            if(QMessageBox::question(this,i18n("Move project to Trash?"),
                i18n("Move \"%1\" into a recoverable trash folder? You can undo this.",
                     file.fileName()),QMessageBox::Yes|QMessageBox::Cancel,
                QMessageBox::Cancel)!=QMessageBox::Yes)return;
            QDir dir(file.absolutePath());
            const QString folder=QStringLiteral(".krita-gallery-trash");
            if(!dir.exists(folder) && !dir.mkdir(folder)) {
                QMessageBox::warning(this,i18n("Project Trash"),
                    i18n("Could not create a recoverable trash folder; original left unchanged."));
                return;
            }
            const QString destination=dir.filePath(folder+QStringLiteral("/")+
                file.fileName()+QStringLiteral(".")+
                QString::number(QDateTime::currentMSecsSinceEpoch()));
            if(!QFile::rename(source,destination)) {
                QMessageBox::warning(this,i18n("Project Trash"),
                    i18n("Could not move project safely; original left unchanged."));
                return;
            }
            QVariantList stack=trashStack();
            QVariantMap item;
            item.insert(QStringLiteral("original"),source);
            item.insert(QStringLiteral("trash"),destination);
            stack.append(item);
            QSettings().setValue(QStringLiteral("TabletUi/galleryTrashStack"),stack);
            syncUndo();
            refreshGallery();
        };
        auto galleryHoldTimer=new QTimer(galleryPage);
        galleryHoldTimer->setSingleShot(true);
        galleryHoldTimer->setInterval(850);
        auto heldIndex=std::make_shared<QPersistentModelIndex>();
        auto heldList=std::make_shared<QPointer<QListWidget>>();
        connect(galleryHoldTimer,&QTimer::timeout,this,
                [this,heldIndex,heldList,suppressGalleryClick,moveToGalleryTrash]() {
            if(!heldList->data() || !heldIndex->isValid())return;
            QListWidget *view=heldList->data();
            const QModelIndex index=*heldIndex;
            const QString path=index.data(Qt::UserRole).toString();
            suppressGalleryClick->insert(view);
            QMenu menu(this);
            QAction *trash=menu.addAction(
                i18n("Move selected project to Trash (undoable)"));
            trash->setIcon(QIcon::fromTheme(QStringLiteral("user-trash")));
            const QPoint menuPos=view->viewport()->mapToGlobal(
                view->visualRect(index).center());
            if(menu.exec(menuPos)==trash)
                moveToGalleryTrash(path);
        });
        for(QListWidget *view:{galleryList,deviceList}) {
            connect(view,&QListWidget::itemPressed,this,
                    [galleryHoldTimer,heldIndex,heldList,suppressGalleryClick,view](
                            QListWidgetItem *item) {
                suppressGalleryClick->remove(view);
                *heldIndex=QPersistentModelIndex(view->model()->index(view->row(item),0));
                *heldList=view;
                galleryHoldTimer->start();
            });
            connect(view,&QListWidget::itemClicked,this,
                    [galleryHoldTimer](QListWidgetItem*) {
                galleryHoldTimer->stop();
            });
            connect(QScroller::scroller(view->viewport()),&QScroller::stateChanged,
                    this,[galleryHoldTimer](QScroller::State state) {
                if(state!=QScroller::Inactive)galleryHoldTimer->stop();
            });
        }
