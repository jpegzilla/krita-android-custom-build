#!/usr/bin/env python3
"""Android gallery: separate selected-folder and accessible-device tabs."""
from pathlib import Path
import sys

from patch_paths import fragment, replace_once


def main(root):
    root=Path(root).resolve()
    path=root/"libs/ui/KisWelcomePageWidget.cpp"
    s=path.read_text(encoding="utf-8")
    s=replace_once(s,"#include <QComboBox>\n",
          "#include <QComboBox>\n#include <QTabWidget>\n#include <QTimer>\n#include <QJsonArray>\n#include <QJsonObject>\n#include <QJsonDocument>\n#include <QScroller>\n#include <QRegularExpression>\n#include <QDateTime>\n",
          "includes")
    s=replace_once(s,
'''QDirIterator iter(*folder, QStringList() << QStringLiteral("*.kra")
                                                        << QStringLiteral("*.krz"),''',
'''QDirIterator iter(*folder, QStringList()
                                                        << QStringLiteral("*.kra*")
                                                        << QStringLiteral("*.krz*"),''',
          "folder compatible formats")
    s=replace_once(s,
'''        auto *galleryList = new QListWidget(galleryPage);''',
'''        auto *galleryTabs = new QTabWidget(galleryPage);
        auto *galleryList = new QListWidget(galleryTabs);
        galleryList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        galleryList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        galleryList->viewport()->setAttribute(Qt::WA_AcceptTouchEvents,true);
        QScroller::grabGesture(galleryList->viewport(),QScroller::TouchGesture);''',
          "create tabs")
    s=replace_once(s,
'''        galleryLayout->addWidget(galleryList);
        pages->addWidget(galleryPage);''',
'''        galleryTabs->addTab(galleryList, i18n("Selected folder"));
        auto *deviceList = new QListWidget(galleryTabs);
        deviceList->setViewMode(QListView::IconMode);
        deviceList->setIconSize(QSize(156,156));
        deviceList->setGridSize(QSize(184,205));
        deviceList->setResizeMode(QListView::Adjust);
        deviceList->setMovement(QListView::Static);
        deviceList->setWordWrap(true);
        deviceList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        deviceList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        deviceList->viewport()->setAttribute(Qt::WA_AcceptTouchEvents,true);
        QScroller::grabGesture(deviceList->viewport(),QScroller::TouchGesture);
        galleryTabs->addTab(deviceList,i18n("Krita projects on device"));
        galleryLayout->addWidget(galleryTabs);
        pages->addWidget(galleryPage);''',"tab widgets")
    s=replace_once(s,
'''        auto populate = [this, galleryList, folder, sortMode]() {''',
'''        auto populate = [this, galleryList, folder, sortMode]() {''',
          "population signature")
    start=s.find("            // Keep SAF content URI projects in reach through Krita recent files.")
    end=s.find("        };\n        connect(galleryButton,", start)
    if start<0 or end<0:
        raise RuntimeError("Gallery tabs: recent SAF block not located")
    s=s[:start]+s[end:]
    device_code=fragment("gallery_tabs/59-auto-updategalleryaccess-all.inc")
    token="        connect(galleryButton, &QToolButton::clicked, this,"
    s=replace_once(s,token,device_code+token,"device listing")
    s=replace_once(s,
'''                [pages, galleryPage, galleryButton, populate, this]() {''',
'''                [pages, galleryPage, galleryButton, galleryTabs, scanTimer,
                 populate, populateDevice, this]() {''',
          "gallery switching capture")
    s=replace_once(s,
'''                pages->setCurrentWidget(galleryPage);
                galleryButton->setText(i18n("Recent Images"));''',
'''                pages->setCurrentWidget(galleryPage);
                if(galleryTabs->currentIndex()==1)populateDevice();
                galleryButton->setText(i18n("Recent Images"));''',
          "device refresh on opening")
    s=replace_once(s,
'''            if (pages->currentWidget() == galleryPage) {
                // Krita's recent-file model''',
'''            if (pages->currentWidget() == galleryPage) {
                scanTimer->stop();
                // Krita's recent-file model''',
          "stop scan on leaving")
    s=replace_once(s,
'''        connect(refreshButton, &QPushButton::clicked, this, populate);''',
'''        connect(refreshButton, &QPushButton::clicked, this,
                [galleryTabs,populate,populateDevice]() {
            if(galleryTabs->currentIndex()==1)populateDevice();
            else populate();
        });''',
          "refresh current tab")
    s=replace_once(s,
'''        connect(sortMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [populate](int) { populate(); });''',
'''        connect(sortMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [galleryTabs,populate,populateDevice](int) {
            if(galleryTabs->currentIndex()==1)populateDevice();
            else populate();
        });''',"sort current tab")
    s=replace_once(s,
'''        connect(galleryList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
            m_mainWindow->openDocument(item->data(Qt::UserRole).toString(), KisMainWindow::None);
        });''',
'''        connect(galleryList, &QListWidget::itemClicked, this,
                [this,galleryList,suppressGalleryClick](QListWidgetItem *item) {
            if(suppressGalleryClick->remove(galleryList))return;
            m_mainWindow->openDocument(item->data(Qt::UserRole).toString(),
                                        KisMainWindow::None);
        });''',"gallery long-press click suppression")
    # The selected-folder tab previously opened up to 2,000 KRA zip files
    # synchronously for thumbnails. Limit candidates and decode only visible
    # previews; the user can refine the selected folder if needed.
    s=replace_once(s,"while (iter.hasNext() && files.size() < 2000)",
            "while (iter.hasNext() && files.size() < 350)","folder scan cap")
    s=replace_once(s,"for (const QFileInfo &file : files) {",
            """int rendered=0;
                for (const QFileInfo &file : files) {
                    if (rendered>=120)break;
                    if (!QRegularExpression(QStringLiteral(
                            "(?i)\\\\.(?:kra|krz)(?: \\\\([0-9]+\\\\))?$"))
                            .match(file.fileName()).hasMatch())continue;
                    ++rendered;""","visible folder cap")
    s=replace_once(s,"if (store && !store->bad()) {",
            "if (rendered<=48 && store && !store->bad()) {",
            "defer large folder previews")
    path.write_text(s,encoding="utf-8")

    # Android MediaStore query complements filesystem scanning; use only
    # normal shared-storage records and retain URI permissions/visibility.
    java_path=root/"packaging/android/apk/src/org/krita/android/MainActivity.java"
    java=java_path.read_text(encoding="utf-8")
    java_code=fragment("gallery_tabs/391-public-static-string-tableti.inc")
    marker="    private static final String TAG"
    java=replace_once(java,marker,java_code+marker,"Java index helper")
    java_path.write_text(java,encoding="utf-8")
    print("Gallery: tabs, indexed Android art and incremental shared-device scan applied")

if __name__=="__main__":
    if len(sys.argv)!=2:
        raise SystemExit("Usage: apply_gallery_tabs.py KRITA_SOURCE")
    main(sys.argv[1])
