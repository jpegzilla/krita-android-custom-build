                const QTransform finalTransform = drag.shape->transformation();
                if (drag.initialTransform != finalTransform) {
                    drag.canvas->addCommand(new KoShapeTransformCommand(
                        {drag.shape}, {drag.initialTransform}, {finalTransform}));
                }
                const bool shortStationaryTap =
                    drag.touchAge.isValid() && drag.touchAge.elapsed() <= 350 &&
                    !drag.moved && !drag.usedTwoFingers &&
                    drag.initialTransform == finalTransform;
                if (shortStationaryTap &&
                    QSettings().value(
                        QStringLiteral("TabletUi/referenceDoubleTapEnabled"),
                        true).toBool()) {
                    const TabletReferenceTap previous = tabletReferenceTaps.value(this);
                    const int maxGapMs = QSettings().value(
                        QStringLiteral("TabletUi/referenceDoubleTapMs"), 350).toInt();
                    if (previous.sinceTap.isValid() &&
                        previous.sinceTap.elapsed() <= qBound(120, maxGapMs, 650) &&
                        previous.shape == drag.shape && previous.canvas == drag.canvas &&
                        QLineF(previous.widgetPoint, drag.startingWidgetPoint).length() <= 36.0) {
                        tabletReferenceTaps.remove(this);
                        const QTransform before = drag.shape->transformation();
                        QTransform flip;
                        flip.translate(drag.shape->size().width(), 0);
                        flip.scale(-1.0, 1.0);
                        drag.shape->update();
                        drag.shape->applyTransformation(flip);
                        drag.shape->update();
                        drag.canvas->addCommand(new KoShapeTransformCommand(
                            {drag.shape}, {before}, {drag.shape->transformation()}));
                    } else {
                        auto &lastTap = tabletReferenceTaps[this];
                        lastTap.shape = drag.shape;
                        lastTap.canvas = drag.canvas;
                        lastTap.widgetPoint = drag.startingWidgetPoint;
                        lastTap.sinceTap.start();
                    }
                } else {
                    tabletReferenceTaps.remove(this);
                }
                if (auto decoration = drag.canvas->referenceImagesDecoration())
                    decoration->invalidateInteractiveReferenceCache();
            }
            d->allowMouseEvents();