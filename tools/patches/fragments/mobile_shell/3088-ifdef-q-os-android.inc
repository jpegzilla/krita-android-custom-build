#ifdef Q_OS_ANDROID
void KisAssistantTool::mobileToggleThreePointPlacement()
{
    if (m_mobilePerspectivePlacement) {
        mobileCancelThreePointPlacement();
        return;
    }
    if (mode()!=HOVER_MODE) return;
    m_newAssistant.clear();
    m_mobilePerspectivePlacement=true;
    m_canvas->viewManager()->showFloatingMessage(
        i18n("Place the first and second vanishing points on the horizon, then the third above or below."),
        QIcon(),3500,KisFloatingMessage::Medium);
    m_canvas->updateCanvas();
}

void KisAssistantTool::mobileCancelThreePointPlacement()
{
    if (!m_mobilePerspectivePlacement) return;
    m_mobilePerspectivePlacement=false;
    m_mobileVanishingPoints.clear();
    setMode(HOVER_MODE);
    m_canvas->updateCanvas();
}

void KisAssistantTool::requestStrokeCancellation()
{
    mobileCancelThreePointPlacement();
    KisTool::requestStrokeCancellation();
}

void KisAssistantTool::requestStrokeEnd()
{
    mobileCancelThreePointPlacement();
    KisTool::requestStrokeEnd();
}

void KisAssistantTool::mobilePlaceVanishingPoint(KoPointerEvent *event)
{
    setMode(HOVER_MODE);
    if (!event) {
        mobileCancelThreePointPlacement();
        return;
    }
    const auto transform=m_canvas->coordinatesConverter()->documentToWidgetTransform();
    QVector<QPointF> placed;
    for (const auto &point:m_mobileVanishingPoints) placed.append(transform.map(point));
    if (!mobileCanPlaceVanishingPoint(placed,transform.map(event->point))) {
        m_canvas->viewManager()->showFloatingMessage(
            i18n("Place the vanishing point farther from the others and off the horizon."),
            QIcon(),2000,KisFloatingMessage::Medium);
        return;
    }
    m_mobileVanishingPoints.append(event->point);
    if (m_mobileVanishingPoints.size()<3) {
        m_canvas->updateCanvas();
        return;
    }

    auto *factory=KisPaintingAssistantFactoryRegistry::instance()->get("vanishing point");
    if (factory) {
        auto decoration=m_canvas->paintingAssistantsDecoration();
        const auto before=KisPaintingAssistant::cloneAssistantList(decoration->assistants());
        for (const QPointF &point:m_mobileVanishingPoints) {
            auto guide=toQShared(factory->createPaintingAssistant());
            guide->setLocal(false);
            guide->addHandle(new KisPaintingAssistantHandle(point),HandleType::NORMAL);
            decoration->addAssistant(guide);
        }
        const auto after=KisPaintingAssistant::cloneAssistantList(decoration->assistants());
        m_canvas->viewManager()->undoAdapter()->addCommand(new EditAssistantsCommand(
            m_canvas,before,after,EditAssistantsCommand::ADD,before.size()));
        m_handles=decoration->handles();
        updateToolOptionsUI();
    }
    mobileCancelThreePointPlacement();
}
#endif

void KisAssistantTool::beginPrimaryAction(KoPointerEvent *event)
{
#ifdef Q_OS_ANDROID
    if (m_mobilePerspectivePlacement) {
        setMode(PAINT_MODE);
        return;
    }
#endif