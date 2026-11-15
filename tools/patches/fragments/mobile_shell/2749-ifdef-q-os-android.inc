#ifdef Q_OS_ANDROID
void KisToolFill::updateMobileFillTolerance(int widgetX)
{
    m_mobilePreviewTolerance = mobileFillTolerance(
        m_mobileInitialTolerance,widgetX - m_fillStartWidgetPosition.x());
    if (m_mobileTolerancePreview) m_mobileTolerancePreview->setThreshold(m_mobilePreviewTolerance);
}

void KisToolFill::clearMobileFillTolerance()
{
    if (!mobileGesturePending()) return;
    m_mobileFillTarget.clear();
    delete m_mobileTolerancePreview.data();
    setMode(KisTool::HOVER_MODE);
}

void KisToolFill::requestStrokeCancellation()
{
    clearMobileFillTolerance();
    KisToolPaint::requestStrokeCancellation();
}

void KisToolFill::requestStrokeEnd()
{
    clearMobileFillTolerance();
    KisToolPaint::requestStrokeEnd();
}
#endif

void KisToolFill::endPrimaryAction(KoPointerEvent *event)
{
#ifdef Q_OS_ANDROID
    if (mobileGesturePending()) {
        const KisNodeSP target = m_mobileFillTarget;
        const QPoint seed = m_mobileFillSeed;
        if (event) updateMobileFillTolerance(event->pos().x());
        clearMobileFillTolerance();
        if (!event || currentNode() != target || !nodeEditable() ||
            nodePaintAbility() != NodePaintAbility::PAINT) return;
        const QScopedValueRollback<int> threshold(m_threshold,m_mobilePreviewTolerance);
        m_seedPoints = {seed};
        beginFilling(seed);
        if (m_fillStrokeId) {
            slotUpdateFill();
            endFilling();
        } else {
            setMode(KisTool::HOVER_MODE);
        }
        m_seedPoints.clear();
        return;
    }
#else
    Q_UNUSED(event);
#endif