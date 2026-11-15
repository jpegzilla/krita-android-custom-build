#ifdef Q_OS_ANDROID
void KisToolSelectContiguous::continuePrimaryAction(KoPointerEvent *event)
{
    if (mobileGesturePending()) {
        if (m_mobilePreview) m_mobilePreview->setThreshold(
            mobileFillTolerance(m_mobileInitialTolerance,event->pos().x()-m_mobileStartX));
        return;
    }
    KisToolSelectBase::continuePrimaryAction(event);
}

void KisToolSelectContiguous::clearMobileTolerance()
{
    if (!mobileGesturePending()) return;
    m_mobileTarget.clear();
    delete m_mobilePreview.data();
    endSelectInteraction();
}

void KisToolSelectContiguous::requestStrokeEnd()
{
    clearMobileTolerance();
    KisToolSelectBase::requestStrokeEnd();
}

void KisToolSelectContiguous::requestStrokeCancellation()
{
    clearMobileTolerance();
    KisToolSelectBase::requestStrokeCancellation();
}
#endif

void KisToolSelectContiguous::endPrimaryAction(KoPointerEvent *event)
{
#ifdef Q_OS_ANDROID
    if (mobileGesturePending()) {
        const auto target=m_mobileTarget;
        if (event && currentNode()==target && target->projection() && selectionEditable()) {
            const int threshold=mobileFillTolerance(m_mobileInitialTolerance,event->pos().x()-m_mobileStartX);
            selectAt(m_mobileSeed,m_mobileSelectionAction,threshold);
        }
        clearMobileTolerance();
        return;
    }
#endif