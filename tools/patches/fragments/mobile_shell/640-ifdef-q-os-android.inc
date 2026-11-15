#ifdef Q_OS_ANDROID
void KisReferenceImagesLayer::paintMobileScreenReferences(QPainter &painter)
{
    auto references = referenceImages();
    std::stable_sort(references.begin(), references.end(), KoShape::compareShapeZIndex);
    for (auto *image : references) {
        if (!image->mobileScreenPinned()) continue;
        painter.save();
        painter.setTransform(image->mobileScreenTransform(), true);
        KoShapeManager::renderSingleShape(image, painter);
        painter.restore();
    }
}

KisReferenceImage *KisReferenceImagesLayer::mobileReferenceAt(const QPointF &point,
                                                              const QTransform &documentToWidget) const
{
    auto references = referenceImages();
    std::stable_sort(references.begin(), references.end(), KoShape::compareShapeZIndex);
    for (bool screen : {true, false}) {
        for (auto it = references.crbegin(); it != references.crend(); ++it) {
            auto *image = *it;
            if (image->mobileScreenPinned() != screen || !image->isVisible()) continue;
            const QTransform transform = screen ? image->mobileScreenTransform() : documentToWidget;
            if (image->hitTest(transform.inverted().map(point))) return image;
        }
    }
    return nullptr;
}

QColor KisReferenceImagesLayer::mobileReferencePixel(const QPointF &point,
                                                      const QTransform &documentToWidget) const
{
    auto *image = mobileReferenceAt(point, documentToWidget);
    if (!image) return {};
    const QTransform transform = image->mobileScreenPinned()
        ? image->mobileScreenTransform() : documentToWidget;
    return image->getPixel(transform.inverted().map(point));
}
#endif

bool KisReferenceImagesLayer::allowAsChild