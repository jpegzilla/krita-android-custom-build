#ifdef Q_OS_ANDROID
    if (!m_view->mainWindow()) {
        KisNodeSP node = selection->parentNode();
        if (!node) return;

        auto *tools = KoToolManager::instance();
        const QString activeId = tools->activeToolId();
        KoToolBase *transform = nullptr;
        auto *nodes = m_view->nodeManager();
        if (activeId.section(QLatin1Char('/'), -1) == QStringLiteral("KisToolTransform") &&
            (node != nodes->activeNode() || nodes->selectedNodes() != KisNodeList{node})) {
            transform = tools->toolById(m_view->canvasBase(), activeId);
            bool canRetarget = false;
            if (!transform ||
                !QMetaObject::invokeMethod(transform, "mobileCanRetargetLayers",
                                           Qt::DirectConnection, Q_RETURN_ARG(bool, canRetarget)) ||
                !canRetarget) {
                m_view->showFloatingMessage(
                    i18n("Apply or reset the current transform before editing the selection."),
                    QIcon(), 3000, KisFloatingMessage::Medium);
                return;
            }
        }

        if (KisNodeManager::isNodeHidden(node, true)) {
            nodes->mobileActivateGlobalSelection(node);
        } else {
            nodes->slotNonUiActivatedNode(node);
        }
        if (nodes->activeNode() != node) return;
        nodes->slotSetSelectedNodes(KisNodeList{node});
        if (transform && !selection->hasShapeSelection()) {
            QMetaObject::invokeMethod(transform, "mobileRetargetLayers", Qt::DirectConnection);
            return;
        }
    } else
#endif
    {
        KisAction *action = m_view->actionManager()->actionByName("show-global-selection-mask");
        KIS_SAFE_ASSERT_RECOVER_RETURN(action);

        if (!action->isChecked()) {
            action->setChecked(true);
            Q_EMIT action->toggled(true);
            Q_EMIT action->triggered(true);
        }

        KisNodeSP node = selection->parentNode();
        KIS_SAFE_ASSERT_RECOVER_RETURN(node);

        m_view->nodeManager()->slotNonUiActivatedNode(node);
    }