#!/usr/bin/env python3
"""v6: Reference-image finger double-tap to flip horizontally; redraw after resize.

Apply after v1..v5 to the pinned KDE Krita v6.0.4 Android source.
Only touch paths for an actual reference image are intercepted.
"""
from pathlib import Path
import sys

from patch_paths import fragment

def main(root):
    changed={}
    def get(p):
        if p not in changed:
            changed[p]=(root/p).read_text(encoding="utf-8")
        return changed[p]
    def sub(p, old, new, expected=1):
        s=get(p)
        n=s.count(old)
        if n!=expected:
            raise RuntimeError(f"v6 {p}: expected {expected} occurrences, found {n}: {old[:110]!r}")
        changed[p]=s.replace(old,new)

    # Krita keeps a cached QImage of reference shapes. On a pinch-in, repaint
    # regions derived from only the new shape bounds can leave remnants of
    # the old image. Invalidate that buffer and repaint the full decoration
    # in its new bounds on interactive reference transforms.
    p="libs/ui/KisReferenceImagesDecoration.h"
    sub(p, """    bool documentHasReferenceImages() const;

private Q_SLOTS:""",
           """    bool documentHasReferenceImages() const;

    /** Force rebuilding the reference overlay after an interactive transform.
     *  The default dirty-rectangle updates can leave remnants after shrinking.
     */
    void invalidateInteractiveReferenceCache();

private Q_SLOTS:""")
    p="libs/ui/KisReferenceImagesDecoration.cpp"
    sub(p, """KisReferenceImagesDecoration::~KisReferenceImagesDecoration()
{}

void KisReferenceImagesDecoration::addReferenceImage""",
           """KisReferenceImagesDecoration::~KisReferenceImagesDecoration()
{}

void KisReferenceImagesDecoration::invalidateInteractiveReferenceCache()
{
    if (!view() || !view()->canvasBase()) return;
    d->buffer.image = QImage();
    d->previousViewSize = QSizeF(); // force full buffer reconstruction on next paint
    view()->canvasBase()->updateCanvasDecorations(); // redraw old and new overlay bounds
}

void KisReferenceImagesDecoration::addReferenceImage""")

    p="libs/ui/input/kis_input_manager.cpp"
    sub(p, """    bool inTwoFinger = false;
};
QHash<KisInputManager*, TabletReferenceDrag> tabletReferenceDrags;""",
           """    bool inTwoFinger = false;
    bool usedTwoFingers = false;
    bool moved = false;
    QPointF startingWidgetPoint;
    QElapsedTimer touchAge;
    QElapsedTimer lastCacheRefresh;
};
struct TabletReferenceTap {
    KoShape *shape = nullptr;
    KisCanvas2 *canvas = nullptr;
    QPointF widgetPoint;
    QElapsedTimer sinceTap;
};
QHash<KisInputManager*, TabletReferenceDrag> tabletReferenceDrags;
QHash<KisInputManager*, TabletReferenceTap> tabletReferenceTaps;""")
    sub(p, """    tabletReferenceDrags.remove(this);
#endif""",
           """    tabletReferenceDrags.remove(this);
    tabletReferenceTaps.remove(this);
#endif""")
    sub(p, """                        drag.initialTransform = shape->transformation();
                        tabletReferenceDrags.insert(this, drag);""",
           """                        drag.initialTransform = shape->transformation();
                        drag.startingWidgetPoint = widgetPoint;
                        drag.touchAge.start();
                        tabletReferenceDrags.insert(this, drag);""")
    sub(p, """                if (points.size() >= 2 &&
                    QSettings().value(QStringLiteral("TabletUi/referenceTwoFinger"), true).toBool()) {""",
           """                if (points.size() >= 2 &&
                    QSettings().value(QStringLiteral("TabletUi/referenceTwoFinger"), true).toBool()) {
                    drag.usedTwoFingers = true;
                    drag.moved = true;""")
    sub(p, """                } else {
                    const QPointF now = toDoc(points.first().pos());
                    if (drag.inTwoFinger) {""",
           """                } else {
                    const QPointF now = toDoc(points.first().pos());
                    // A finger never lands perfectly motionless on a
                    // touch sensor. Do not mutate the KoShape at all for
                    // sub-slop movement: even a fractional-pixel move
                    // changes its transform and used to reject double taps.
                    if (QLineF(points.first().pos(), drag.startingWidgetPoint).length() > 12.0)
                        drag.moved = true;
                    if (drag.inTwoFinger) {""")
    sub(p, """                    drag.shape->setAbsolutePosition(
                        drag.startingShapePosition + now - drag.startingDocPoint);
                    drag.shape->update();
                }
            }
            event->accept();""",
           """                    if (drag.moved && QSettings().value(
                            QStringLiteral("TabletUi/referenceFingerDrag"),
                            true).toBool()) {
                        drag.shape->setAbsolutePosition(
                            drag.startingShapePosition + now - drag.startingDocPoint);
                        drag.shape->update();
                    }
                }
                if (!drag.lastCacheRefresh.isValid() ||
                    drag.lastCacheRefresh.elapsed() >= 25) {
                    if (auto decoration = drag.canvas->referenceImagesDecoration())
                        decoration->invalidateInteractiveReferenceCache();
                    drag.lastCacheRefresh.start();
                }
            }
            event->accept();""")
    # The reference gesture is intercepted before Krita's generic touch
    # matcher, so a double tap on the image flips only the image; it never
    # invokes Mirror View on the overall canvas.
    sub(p, """                const QTransform finalTransform = drag.shape->transformation();
                if (drag.initialTransform != finalTransform) {
                    drag.canvas->addCommand(new KoShapeTransformCommand(
                        {drag.shape}, {drag.initialTransform}, {finalTransform}));
                }
            }
            d->allowMouseEvents();""",
           fragment("reference_touch_fix/137-const-qtransform-finaltransf.inc"))
    sub(p, """                drag.shape->setTransformation(drag.initialTransform);
                drag.shape->update();
            }
            d->allowMouseEvents();""",
           """                drag.shape->setTransformation(drag.initialTransform);
                drag.shape->update();
                if (drag.canvas) {
                    if (auto decoration = drag.canvas->referenceImagesDecoration())
                        decoration->invalidateInteractiveReferenceCache();
                }
            }
            tabletReferenceTaps.remove(this);
            d->allowMouseEvents();""")

    for p,content in changed.items():
        (root/p).write_text(content,encoding="utf-8")
        print("v6 reference patch:",p)

if __name__=="__main__":
    if len(sys.argv)!=2: raise SystemExit("usage: apply_reference_touch_fix.py KRITA_SOURCE")
    main(Path(sys.argv[1]))
