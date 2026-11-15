#ifdef Q_OS_ANDROID
QTransform KisReferenceImage::mobileScreenTransform() const { return d->mobileScreenTransform.value_or(QTransform()); }

bool KisReferenceImage::mobileScreenPinned() const { return d->mobileScreenTransform.has_value(); }

void KisReferenceImage::setMobileScreenTransform(const QTransform &transform, bool pinned)
{
    if (pinned) d->mobileScreenTransform = transform;
    else d->mobileScreenTransform.reset();
}

KUndo2Command *KisReferenceImage::mobileToggleScreenPin(KisReferenceImage *reference,
                                                        const QTransform &documentToWidget)
{
    class PinCommand final : public KUndo2Command
    {
    public:
        PinCommand(KisReferenceImage *image, const QTransform &screen, const QTransform &shape)
            : KUndo2Command(kundo2_i18n("Change reference attachment"))
            , image(image), oldScreen(image->mobileScreenTransform()), screen(screen)
            , oldShape(image->transformation()), shape(shape), wasPinned(image->mobileScreenPinned()) {}
        void undo() override { apply(oldScreen, oldShape, wasPinned); }
        void redo() override { apply(screen, shape, !wasPinned); }
    private:
        void apply(const QTransform &screenTransform, const QTransform &shapeTransform, bool pinned)
        {
            image->update();
            image->setMobileScreenTransform(screenTransform, pinned);
            image->setTransformation(shapeTransform);
            image->update();
        }
        KisReferenceImage *image;
        QTransform oldScreen, screen, oldShape, shape;
        bool wasPinned;
    };
    if (!reference->mobileScreenPinned()) {
        return new PinCommand(reference, documentToWidget, reference->transformation());
    }
    bool invertible = false;
    const QTransform inverse = documentToWidget.inverted(&invertible);
    if (!invertible) return nullptr;
    const QTransform local = reference->transformation();
    const QTransform absolute = reference->absoluteTransformation();
    const QTransform parent = local.inverted() * absolute;
    const QTransform attached = absolute * reference->mobileScreenTransform() * inverse * parent.inverted();
    return new PinCommand(reference, QTransform(), attached);
}
#endif

void KisReferenceImage::saveXml(