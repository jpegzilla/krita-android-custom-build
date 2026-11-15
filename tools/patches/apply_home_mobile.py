#!/usr/bin/env python3
"""Android-only touch-first Krita home and searchable/starred project gallery."""
from pathlib import Path
import sys

from patch_paths import fragment, replace_once


def main(root):
    path=Path(root)/"libs/ui/KisWelcomePageWidget.cpp"
    s=path.read_text(encoding="utf-8")
    s=replace_once(s,"#include <QTabWidget>\n",
          "#include <QTabWidget>\n#include <QTabBar>\n#include <QGridLayout>\n"
          "#include <QLineEdit>\n#include <QClipboard>\n"
          "#include <QAbstractItemModel>\n","includes")
    # Controls in two columns, so nothing is cut off in folded portrait.
    s=replace_once(s,"auto *galleryControls = new QHBoxLayout;",
          "auto *galleryControls = new QGridLayout;","gallery toolbar")
    for name,position in (
        ("folderButton","0,0"),("downloadsButton","0,1"),
        ("refreshButton","1,0"),("sortMode","1,1"),
        ("allowFiles","2,0,1,2"),("undoTrash","3,0,1,2")):
        s=replace_once(s,f"galleryControls->addWidget({name});",
              f"galleryControls->addWidget({name},{position});",
              f"toolbar {name}")
    s=replace_once(s,
'''        galleryTabs->addTab(deviceList,i18n("Krita projects on device"));
        galleryLayout->addWidget(galleryTabs);''',
'''        galleryTabs->addTab(deviceList,i18n("On device"));
        galleryLayout->addWidget(galleryTabs);
        galleryLayout->setContentsMargins(4,4,4,4);
        galleryLayout->setSpacing(7);
        galleryTabs->setStyleSheet(
            "QTabBar::tab{min-height:45px;padding:0 12px;font-size:13px;}"
            "QTabWidget::pane{border:0;}");
        for(QWidget *control:{static_cast<QWidget*>(folderButton),
                               static_cast<QWidget*>(downloadsButton),
                               static_cast<QWidget*>(refreshButton),
                               static_cast<QWidget*>(sortMode),
                               static_cast<QWidget*>(allowFiles)}) {
            control->setMinimumHeight(45);
            control->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        }
        for(QListWidget *list:{galleryList,deviceList}) {
            list->setIconSize(QSize(140,140));
            list->setGridSize(QSize(164,186));
            list->setSpacing(3);
            list->setMinimumWidth(0);
        }''',"touch gallery controls")
    filter_code=fragment("home_mobile/50-auto-searchrow-new-qhboxlayo.inc")
    s=replace_once(s,"        auto suppressGalleryClick=std::make_shared<QSet<QListWidget*>>();",
          filter_code+"        auto suppressGalleryClick=std::make_shared<QSet<QListWidget*>>();",
          "search and starred filter")
    s=replace_once(s,
'''                [this,heldIndex,heldList,suppressGalleryClick,moveToGalleryTrash]() {''',
'''                [this,heldIndex,heldList,suppressGalleryClick,moveToGalleryTrash,
                 favorites,scheduleFilter]() {''',
          "long press captures")
    s=replace_once(s,
'''            QAction *trash=menu.addAction(
                i18n("Move selected project to Trash (undoable)"));
            trash->setIcon(QIcon::fromTheme(QStringLiteral("user-trash")));
            const QPoint menuPos=view->viewport()->mapToGlobal(
                view->visualRect(index).center());
            if(menu.exec(menuPos)==trash)
                moveToGalleryTrash(path);''',
'''            QAction *star=menu.addAction(favorites->contains(path)?
                i18n("Remove from starred projects"):i18n("Star this project"));
            QAction *copy=menu.addAction(i18n("Copy project location"));
            menu.addSeparator();
            QAction *trash=menu.addAction(
                i18n("Move selected project to Trash (undoable)"));
            trash->setIcon(QIcon::fromTheme(QStringLiteral("user-trash")));
            const QPoint menuPos=view->viewport()->mapToGlobal(
                view->visualRect(index).center());
            QAction *selected=menu.exec(menuPos);
            if(selected==star) {
                if(favorites->contains(path))favorites->remove(path);
                else favorites->insert(path);
                QStringList values;
                for(const QString &name:*favorites)values.append(name);
                QSettings().setValue(QStringLiteral("TabletUi/starredProjectsV1"),
                                     values);
                scheduleFilter();
            } else if(selected==copy) {
                QGuiApplication::clipboard()->setText(path);
            } else if(selected==trash) {
                moveToGalleryTrash(path);
            }''',"long press project options")
    home=fragment("home_mobile/168-android-only-start-panel-ret.inc")
    s=replace_once(s,'        connect(folderButton, &QPushButton::clicked, this,',
          home+'        connect(folderButton, &QPushButton::clicked, this,',
          "Android main start panel")
    # Consolidate gallery into a single On Device screen. The old selected
    # folder remains an internal fallback source, not a navigation tab.
    s=replace_once(s,'        galleryLayout->addWidget(galleryTabs);',
'''        galleryLayout->addWidget(galleryTabs);
        galleryTabs->setCurrentIndex(1);
        galleryTabs->tabBar()->hide();''',"single on-device gallery")
    # Hide obsolete controls without changing the tested file scan/permissions
    # logic; users retain access permissions if needed.
    s=replace_once(s,'        syncUndo();\n        auto refreshGallery=',
'''        syncUndo();
        galleryControls->removeWidget(folderButton);
        galleryControls->removeWidget(downloadsButton);
        folderButton->hide();
        downloadsButton->hide();
        galleryControls->removeWidget(sortMode);
        galleryControls->removeWidget(refreshButton);
        galleryControls->removeWidget(undoTrash);
        galleryControls->removeWidget(allowFiles);
        galleryControls->addWidget(sortMode,0,1);
        galleryControls->addWidget(refreshButton,0,2);
        galleryControls->addWidget(undoTrash,0,3);
        galleryControls->addWidget(allowFiles,1,0,1,4);
        galleryControls->setColumnStretch(0,1);
        galleryControls->setContentsMargins(0,0,0,0);
        galleryControls->setSpacing(4);
        sortMode->setItemText(0,i18n("Newest"));
        sortMode->setItemText(1,i18n("A-Z"));
        sortMode->setFixedWidth(110);
        sortMode->setMinimumHeight(42);
        refreshButton->setText(QStringLiteral("↻"));
        refreshButton->setAccessibleName(i18n("Refresh projects"));
        refreshButton->setToolTip(i18n("Rescan projects on device"));
        refreshButton->setFixedSize(43,42);
        refreshButton->setStyleSheet("QPushButton{font-size:23px;}");
        undoTrash->setText(i18n("Undo"));
        undoTrash->setToolTip(i18n("Restore last trashed project"));
        undoTrash->setFixedSize(65,42);
        allowFiles->setMinimumHeight(44);
        auto refreshGallery=''',"compact gallery toolbar")
    s=replace_once(s,'''            undoTrash->setEnabled(!stack.isEmpty() &&
                QFileInfo(stack.last().toMap().value(
                    QStringLiteral("trash")).toString()).isFile());''',
'''            undoTrash->setEnabled(!stack.isEmpty() &&
                QFileInfo(stack.last().toMap().value(
                    QStringLiteral("trash")).toString()).isFile());
            undoTrash->setVisible(undoTrash->isEnabled());''',
          "contextual undo button")
    # Save timestamp for reliable newest-first mode without opening project
    # archives just to sort. Alphabetical mode is case-insensitive.
    s=replace_once(s,'            item->setData(Qt::UserRole,location);',
'''            item->setData(Qt::UserRole,location);
            item->setData(Qt::UserRole+5,physicalPath.isEmpty() ? 0LL :
                QFileInfo(physicalPath).lastModified().toMSecsSinceEpoch());''',
          "project modification times")
    s=replace_once(s,'        auto *searchRow=new QHBoxLayout;',
'''        const auto sortDevice=[deviceList,sortMode]() {
            if(deviceList->count()<2)return;
            QList<QListWidgetItem*> sorted;
            sorted.reserve(deviceList->count());
            deviceList->setUpdatesEnabled(false);
            while(deviceList->count())sorted.append(deviceList->takeItem(0));
            std::stable_sort(sorted.begin(),sorted.end(),
                [sortMode](QListWidgetItem *a,QListWidgetItem *b) {
                    if(sortMode->currentIndex()==1) {
                        const QString an=a->data(Qt::UserRole+4).toString();
                        const QString bn=b->data(Qt::UserRole+4).toString();
                        return QString::compare(an.isEmpty()?a->text():an,
                            bn.isEmpty()?b->text():bn,Qt::CaseInsensitive)<0;
                    }
                    const qint64 am=a->data(Qt::UserRole+5).toLongLong();
                    const qint64 bm=b->data(Qt::UserRole+5).toLongLong();
                    return am==bm ?
                        QString::compare(a->text(),b->text(),
                                         Qt::CaseInsensitive)<0 : am>bm;
                });
            for(QListWidgetItem *item:sorted)deviceList->addItem(item);
            deviceList->setUpdatesEnabled(true);
        };
        // Existing incremental device scanner runs first on timeout. When
        // it stops (end of scan), sort the completed list only once.
        connect(scanTimer,&QTimer::timeout,this,[scanTimer,sortDevice]() {
            if(!scanTimer->isActive())sortDevice();
        });
        auto *searchRow=new QHBoxLayout;''',"device sorting")
    # The gallery is a project library, not a one-tap launcher. Both touch
    # and long-press now open the same spacious, touch-friendly project sheet.
    s=replace_once(s,
'''        auto suppressGalleryClick=std::make_shared<QSet<QListWidget*>>();''',
'''        auto projectSheet=std::make_shared<
            std::function<void(QListWidgetItem*)>>();
        auto suppressGalleryClick=std::make_shared<QSet<QListWidget*>>();''',
        "project sheet dispatcher")
    s=replace_once(s,
'''        connect(deviceList,&QListWidget::itemClicked,this,
                [this,deviceList,suppressGalleryClick](QListWidgetItem *item) {
            if(suppressGalleryClick->remove(deviceList))return;
            m_mainWindow->openDocument(item->data(Qt::UserRole).toString(),
                                        KisMainWindow::None);
        });''',
'''        connect(deviceList,&QListWidget::itemClicked,this,
                [deviceList,suppressGalleryClick,projectSheet](QListWidgetItem *item) {
            if(suppressGalleryClick->remove(deviceList))return;
            if(*projectSheet)(*projectSheet)(item);
        });''',"device tap opens project sheet")
    s=replace_once(s,
'''        connect(galleryList, &QListWidget::itemClicked, this,
                [this,galleryList,suppressGalleryClick](QListWidgetItem *item) {
            if(suppressGalleryClick->remove(galleryList))return;
            m_mainWindow->openDocument(item->data(Qt::UserRole).toString(),
                                        KisMainWindow::None);
        });''',
'''        connect(galleryList, &QListWidget::itemClicked, this,
                [galleryList,suppressGalleryClick,projectSheet](QListWidgetItem *item) {
            if(suppressGalleryClick->remove(galleryList))return;
            if(*projectSheet)(*projectSheet)(item);
        });''',"folder tap opens project sheet")
    sheet=fragment("home_mobile/415-projectsheet-this-favorites-.inc")
    s=replace_once(s,'''        auto galleryHoldTimer=new QTimer(galleryPage);''',
          sheet+'''        auto galleryHoldTimer=new QTimer(galleryPage);''',
          "project sheet definition")
    s=replace_once(s,
'''                [this,heldIndex,heldList,suppressGalleryClick,moveToGalleryTrash,
                 favorites,scheduleFilter]() {''',
'''                [heldIndex,heldList,suppressGalleryClick,projectSheet]() {''',
          "long press project captures")
    s=replace_once(s,
'''            const QString path=index.data(Qt::UserRole).toString();
            suppressGalleryClick->insert(view);
            QMenu menu(this);
            QAction *star=menu.addAction(favorites->contains(path)?
                i18n("Remove from starred projects"):i18n("Star this project"));
            QAction *copy=menu.addAction(i18n("Copy project location"));
            menu.addSeparator();
            QAction *trash=menu.addAction(
                i18n("Move selected project to Trash (undoable)"));
            trash->setIcon(QIcon::fromTheme(QStringLiteral("user-trash")));
            const QPoint menuPos=view->viewport()->mapToGlobal(
                view->visualRect(index).center());
            QAction *selected=menu.exec(menuPos);
            if(selected==star) {
                if(favorites->contains(path))favorites->remove(path);
                else favorites->insert(path);
                QStringList values;
                for(const QString &name:*favorites)values.append(name);
                QSettings().setValue(QStringLiteral("TabletUi/starredProjectsV1"),
                                     values);
                scheduleFilter();
            } else if(selected==copy) {
                QGuiApplication::clipboard()->setText(path);
            } else if(selected==trash) {
                moveToGalleryTrash(path);
            }''',
'''            suppressGalleryClick->insert(view);
            QListWidgetItem *item=view->item(index.row());
            if(item && *projectSheet)(*projectSheet)(item);''',
          "long press uses same project sheet")
    path.write_text(s,encoding="utf-8")
    print("Mobile home: quick actions, continue, responsive gallery, search, starred projects")

if __name__=="__main__":
    if len(sys.argv)!=2:raise SystemExit("Usage: apply_home_mobile.py KRITA_SOURCE")
    main(sys.argv[1])
