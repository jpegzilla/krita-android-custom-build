            if (drag.canvas && drag.shape && !touchEvent->touchPoints().isEmpty()) {
                auto toDoc = [&drag](const QPointF &point) {
                    return drag.canvas->coordinatesConverter()->widgetToDocument(point);
                };
                const auto &points = touchEvent->touchPoints();
                if (points.size() >= 2 &&
                    QSettings().value(QStringLiteral("TabletUi/referenceTwoFinger"), true).toBool()) {
                    const QPointF first = toDoc(points.at(0).pos());
                    const QPointF second = toDoc(points.at(1).pos());
                    const QPointF mid = 0.5 * (first + second);
                    const QLineF line(first, second);
                    if (!drag.inTwoFinger) {
                        drag.inTwoFinger = true;
                        drag.twoFingerBase = drag.shape->transformation();
                        drag.startCenter = mid;
                        drag.startDistance = qMax(qreal(1.0), line.length());
                        drag.startAngle = line.angle();
                    } else {
                        const qreal factor = qBound(qreal(0.15),
                             line.length() / drag.startDistance, qreal(6.0));
                        const qreal angle = drag.startAngle - line.angle();
                        QTransform gesture;
                        gesture.translate(mid.x(), mid.y());
                        gesture.rotate(angle);
                        gesture.scale(factor, factor);
                        gesture.translate(-drag.startCenter.x(), -drag.startCenter.y());
                        drag.shape->update();
                        drag.shape->setTransformation(drag.twoFingerBase);
                        drag.shape->applyAbsoluteTransformation(gesture);
                        drag.shape->update();
                    }
                } else {
                    const QPointF now = toDoc(points.first().pos());
                    if (drag.inTwoFinger) {
                        drag.inTwoFinger = false;
                        drag.startingShapePosition = drag.shape->absolutePosition();
                        drag.startingDocPoint = now;
                    }
                    drag.shape->update();
                    drag.shape->setAbsolutePosition(
                        drag.startingShapePosition + now - drag.startingDocPoint);
                    drag.shape->update();
                }
            }