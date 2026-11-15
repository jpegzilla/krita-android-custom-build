    QMenu *quickMenu = new QMenu(i18n("Quick Actions"), this);
    quickMenu->menuAction()->setIcon(KisIconUtils::loadIcon("application-menu"));
    actionCollection()->addAction(QStringLiteral("tablet_quick_actions"), quickMenu->menuAction());
    const QStringList tabletQuickAvailable = {
        QStringLiteral("gradients"), QStringLiteral("patterns"),
        QStringLiteral("file_new"), QStringLiteral("file_open"),
        QStringLiteral("file_save_as"), QStringLiteral("file_export_file"),
        QStringLiteral("select_all"), QStringLiteral("invert_selection"),
        QStringLiteral("reselect"), QStringLiteral("deselect"),
        QStringLiteral("fill_selection_foreground_color"),
        QStringLiteral("cut_selection_to_new_layer"),
        QStringLiteral("duplicate_layer"), QStringLiteral("merge_layer"),
        QStringLiteral("quick_group"), QStringLiteral("edit_undo"),
        QStringLiteral("edit_redo"), QStringLiteral("tablet_reset_view"),
        QStringLiteral("tablet_settings")
    };
    const QStringList tabletQuickDefaults = {
        QStringLiteral("gradients"), QStringLiteral("patterns"),
        QStringLiteral("duplicate_layer"), QStringLiteral("quick_group"),
        QStringLiteral("tablet_settings")
    };
    QAction *customizeQuick = new QAction(i18n("Customize Quick Actions..."), this);
    connect(customizeQuick, &QAction::triggered, this,
            [this, tabletQuickAvailable, tabletQuickDefaults]() {
        QDialog dialog(this);
        dialog.setWindowFlags(Qt::Popup|Qt::FramelessWindowHint);
        dialog.setWindowTitle(i18n("Customize Quick Actions"));
        auto *layout = new QVBoxLayout(&dialog);
        auto *info = new QLabel(i18n("Check actions to show. Drag rows to change their order."), &dialog);
        layout->addWidget(info);
        auto *list = new QListWidget(&dialog);
        list->setDragDropMode(QAbstractItemView::InternalMove);
        list->setDefaultDropAction(Qt::MoveAction);
        layout->addWidget(list);
        QStringList selected = QSettings().value(
            QStringLiteral("TabletUi/quickActionsV2"), tabletQuickDefaults).toStringList();
        QStringList ordered = selected;
        for (const QString &name : tabletQuickAvailable) {
            if (!ordered.contains(name)) ordered.append(name);
        }
        for (const QString &name : ordered) {
            if (!tabletQuickAvailable.contains(name)) continue;
            QAction *action = actionCollection()->action(name);
            QString label = action && !action->text().isEmpty() ? action->text() : name;
            auto *item = new QListWidgetItem(label.remove(QLatin1Char('&')), list);
            item->setData(Qt::UserRole, name);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled);
            item->setCheckState(selected.contains(name) ? Qt::Checked : Qt::Unchecked);
        }
        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() == QDialog::Accepted) {
            QStringList chosen;
            for (int i = 0; i < list->count(); ++i) {
                auto *item = list->item(i);
                if (item->checkState() == Qt::Checked)
                    chosen << item->data(Qt::UserRole).toString();
            }
            QSettings().setValue(QStringLiteral("TabletUi/quickActionsV2"), chosen);
        }
    });
    connect(quickMenu, &QMenu::aboutToShow, this,
            [this, quickMenu, customizeQuick, tabletQuickDefaults]() {
        quickMenu->clear();
        const QStringList selected = QSettings().value(
            QStringLiteral("TabletUi/quickActionsV2"), tabletQuickDefaults).toStringList();
        for (const QString &name : selected) {
            QAction *action = actionCollection()->action(name);
            if (action) quickMenu->addAction(action);
        }
        quickMenu->addSeparator();
        quickMenu->addAction(customizeQuick);
    });
