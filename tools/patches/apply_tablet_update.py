#!/usr/bin/env python3
"""Second-stage Android tablet UX customization for Krita v6.0.4.

Run *after* apply_customizations.py. Patches the checked-out Krita tree,
not the copy in the Play Store. The new controls are Android-only.
"""
import sys
from pathlib import Path

from patch_paths import fragment

def main(root: Path):
    modified = {}
    def change(path, old, new, expected=1):
        src = modified.get(path)
        if src is None:
            src = (root / path).read_text(encoding="utf-8")
        count = src.count(old)
        if count != expected:
            raise RuntimeError(f"{path}: expected {expected} instances of {old[:85]!r}, got {count}")
        modified[path] = src.replace(old, new)

    # Separate, persistent timing control for the existing one-finger
    # double-tap matcher. Default 250ms instead of the Android/Qt interval.
    path = "libs/ui/input/kis_shortcut_matcher.cpp"
    change(path, "#include <QTimer>\n", "#include <QTimer>\n#include <QSettings>\n")
    change(path, "QApplication::doubleClickInterval()",
           'qBound(120, QSettings().value(QStringLiteral("TabletUi/doubleTapMs"), 250).toInt(), 600)',
           expected=2)

    # The normal sampler can still be used continuously. Our new action
    # selects the sampler for precisely one stylus contact, then returns to
    # the freehand brush; the tap never paints a brush stroke.
    path = "plugins/tools/basictools/kis_tool_colorsampler.cc"
    change(path, "#include <QSignalBlocker>\n",
           "#include <QSignalBlocker>\n#include <QSettings>\n#include <QTimer>\n#include <QAction>\n#include <kactioncollection.h>\n")
    change(path, """    m_isActivated = true;
    m_config->load();

    updateOptionWidget();""",
           """    m_isActivated = true;
    m_config->load();
#ifdef Q_OS_ANDROID
    m_config->sampleMerged = QSettings().value(QStringLiteral("TabletUi/sampleMerged"), true).toBool();
#endif

    updateOptionWidget();""")
    change(path, """    m_helper.deactivate();
    requestUpdateOutline(event->point, event);

}""",
           """    m_helper.deactivate();
    requestUpdateOutline(event->point, event);

#ifdef Q_OS_ANDROID
    QSettings tabletSettings;
    if (tabletSettings.value(QStringLiteral("TabletUi/oneShotSampler"), false).toBool()) {
        tabletSettings.setValue(QStringLiteral("TabletUi/oneShotSampler"), false);
        QTimer::singleShot(0, this, [this]() {
            KisCanvas2 *kisCanvas = dynamic_cast<KisCanvas2 *>(canvas());
            if (!kisCanvas || !kisCanvas->viewManager()) return;
            QAction *brushAction = kisCanvas->viewManager()->actionCollection()->action(
                QStringLiteral("KritaShape/KisToolBrush"));
            if (brushAction) brushAction->trigger();
        });
    }
#endif
}""")

    # The custom actions use Krita's existing command implementations.
    # We do not replace window state/Android immersive mode when Compact UI
    # is toggled, avoiding the status bar regression in Canvas Only mode.
    path = "libs/ui/KisMainWindow.cpp"
    change(path, "#include <QActionGroup>\n", """#include <QActionGroup>
#ifdef Q_OS_ANDROID
#include <QSettings>
#include <QTimer>
#include <QSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QVBoxLayout>
#endif
""")
    change(path, """    d->fullScreenMode = actionManager->createStandardAction(KStandardAction::FullScreen, this, SLOT(viewFullscreen(bool)));

    d->recentFiles""",
           fragment("tablet_update/86-d-fullscreenmode-actionmanag.inc"))

    # Keep brush opacity and size sliders in their original tool bar. The
    # Android preset makes common actions visible without crowding those sliders.
    path = "krita/krita5.xmlgui"
    change(path, """  <ToolBar name="mainToolBar" fullWidth="false" noMerge="1">
    <Text>File</Text>
    <Action name="file_new"/>
    <Action name="file_open"/>
    <Action name="file_save"/>
  </ToolBar>
  <ToolBar name="editToolBar" fullWidth="false" noMerge="1">
    <Text>Edit</Text>
    <Action name="edit_undo"/>
    <Action name="edit_redo"/>
  </ToolBar>""", """  <ToolBar name="mainToolBar" fullWidth="false" noMerge="1">
    <Text>File</Text>
    <Action name="file_save"/>
    <Action name="deselect"/>
    <Action name="toggle_layer_alpha_lock"/>
    <Action name="tablet_compact_ui"/>
    <Action name="tablet_quick_actions"/>
  </ToolBar>
  <ToolBar name="editToolBar" fullWidth="false" noMerge="1">
    <Text>Edit</Text>
    <Action name="tablet_pick_once"/>
    <Action name="tablet_sample_merged"/>
    <Action name="view_toggle_reference_images"/>
    <Action name="tablet_reset_view"/>
    <Action name="tablet_settings"/>
  </ToolBar>""")
    change(path, """    <Action name="brushslider2"/>
    <Action name="mirror_actions"/>""",
                 """    <Action name="brushslider2"/>
    <Action name="reload_preset_action"/>
    <Action name="mirror_actions"/>""")

    # Krita deliberately checks the XMLGUI toolbar layout checksum during
    # CMake configuration. This custom toolbar requires a new Android-only
    # XMLGUI version and the matching whitespace-stripped MD5.
    path = "krita/CMakeLists.txt"
    change(path, "set(KRITA5_XMLGUI_VERSION 544)",
           "set(KRITA5_XMLGUI_VERSION 545)")
    change(path,
           'set(krita5_xmlgui_hash_expected "b74fdc4f3bafac700be2db906a424542")',
           'set(krita5_xmlgui_hash_expected "a4656de215838ff1371d720f9c3d4b26")')

    # Native Android hardware buttons, implemented on Qt's GUI thread.
    path = "packaging/android/apk/src/org/krita/android/JNIWrappers.java"
    change(path, "    public static native boolean hasMainWindowLoaded();",
           """    public static native boolean hasMainWindowLoaded();
    public static native boolean volumeMappingEnabled();
    public static native void volumeUndoRedo(boolean redo);""")
    path = "packaging/android/apk/src/org/krita/android/MainActivity.java"
    change(path, """    @Override
    public boolean onKeyUp(final int keyCode, final KeyEvent event) {""",
           """    @Override
    public boolean onKeyDown(final int keyCode, final KeyEvent event) {
        if ((keyCode == KeyEvent.KEYCODE_VOLUME_DOWN || keyCode == KeyEvent.KEYCODE_VOLUME_UP)
                && haveLibsLoaded && JNIWrappers.volumeMappingEnabled()) {
            if (event.getRepeatCount() == 0) {
                JNIWrappers.volumeUndoRedo(keyCode == KeyEvent.KEYCODE_VOLUME_UP);
            }
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(final int keyCode, final KeyEvent event) {
        if ((keyCode == KeyEvent.KEYCODE_VOLUME_DOWN || keyCode == KeyEvent.KEYCODE_VOLUME_UP)
                && haveLibsLoaded && JNIWrappers.volumeMappingEnabled()) {
            return true;
        }""")
    path = "krita/main.cc"
    change(path, """#ifdef Q_OS_ANDROID
#include <QtAndroid>""",
           """#ifdef Q_OS_ANDROID
#include <QSettings>
#include <QtAndroid>""")
    change(path, """#define MAIN_EXPORT __attribute__ ((visibility ("default")))
#define MAIN_FN main""",
           """extern "C" JNIEXPORT jboolean JNICALL
Java_org_krita_android_JNIWrappers_volumeMappingEnabled(JNIEnv*, jobject)
{
    return QSettings().value(QStringLiteral("TabletUi/volumeKeys"), true).toBool();
}

extern "C" JNIEXPORT void JNICALL
Java_org_krita_android_JNIWrappers_volumeUndoRedo(JNIEnv*, jobject, jboolean redo)
{
    if (!KisPart::exists()) return;
    KisMainWindow *window = KisPart::instance()->currentMainwindow();
    if (window) {
        QMetaObject::invokeMethod(window, redo ? "redo" : "undo", Qt::QueuedConnection);
    }
}

#define MAIN_EXPORT __attribute__ ((visibility ("default")))
#define MAIN_FN main""")

    # Increase Android selection bar touch targets; pin it to the bottom in
    # compact mode. Uses Krita's existing selection actions and undo logic.
    path = "libs/ui/kis_selection_actions_panel.cpp"
    change(path, """static constexpr int BUTTON_SIZE = 30;""",
           """#ifdef Q_OS_ANDROID
static constexpr int BUTTON_SIZE = 44;
#else
static constexpr int BUTTON_SIZE = 30;
#endif""")
    # Krita already has a fixed-bottom behavior. Keep its users' normal
    # settings when not in Compact UI.
    change(path, """    configChanged(true);

}""",
           """    configChanged(true);

}""") if False else None

    # Optional quick two-finger inward pinch; the original pinch gesture still
    # runs normally. The reset is applied only at gesture completion, after an
    # unmistakable rapid inward movement. Defaults OFF until user opts in.
    path = "libs/ui/input/kis_input_manager.cpp"
    change(path, "#include <QTouchEvent>\n", """#include <QTouchEvent>
#ifdef Q_OS_ANDROID
#include <QSettings>
#include <QElapsedTimer>
#include <QLineF>
#include <QTimer>
#include <QHash>
#include <KisPart.h>
#include <KisMainWindow.h>
#include <kactioncollection.h>
#endif
""")
    change(path, """KisInputManager::KisInputManager(QObject *parent)
""", """#ifdef Q_OS_ANDROID
namespace {
struct TabletPinchTracking {
    QElapsedTimer age;
    qreal startDistance = 0;
    qreal smallestDistance = 0;
    bool tracked = false;
};
QHash<KisInputManager*, TabletPinchTracking> tabletPinches;
}
#endif

KisInputManager::KisInputManager(QObject *parent)
""")
    change(path, """KisInputManager::~KisInputManager()
{
""", """KisInputManager::~KisInputManager()
{
#ifdef Q_OS_ANDROID
    tabletPinches.remove(this);
#endif
""")
    change(path, """    case QEvent::TouchBegin:
    {
        d->debugEvent<QTouchEvent, false>(event);""", """    case QEvent::TouchBegin:
    {
#ifdef Q_OS_ANDROID
        tabletPinches.remove(this);
#endif
        d->debugEvent<QTouchEvent, false>(event);""")
    change(path, """        QTouchEvent *touchEvent = static_cast<QTouchEvent*>(event);
        d->debugEvent<QTouchEvent, false>(event);

        int eventPointCount = touchEvent->touchPoints().size();""",
           """        QTouchEvent *touchEvent = static_cast<QTouchEvent*>(event);
        d->debugEvent<QTouchEvent, false>(event);
#ifdef Q_OS_ANDROID
        if (QSettings().value(QStringLiteral("TabletUi/quickPinch"), false).toBool()
            && touchEvent->touchPoints().size() == 2) {
            const auto &points = touchEvent->touchPoints();
            qreal distance = QLineF(points.at(0).pos(), points.at(1).pos()).length();
            TabletPinchTracking &tracker = tabletPinches[this];
            if (!tracker.tracked && distance > 115) {
                tracker.tracked = true;
                tracker.startDistance = distance;
                tracker.smallestDistance = distance;
                tracker.age.start();
            } else if (tracker.tracked) {
                tracker.smallestDistance = qMin(tracker.smallestDistance, distance);
            }
        }
#endif

        int eventPointCount = touchEvent->touchPoints().size();""")
    change(path, """        retval = d->matcher.touchEndEvent(touchEvent);
        if (d->touchStrokeStarted) {""",
           """#ifdef Q_OS_ANDROID
        auto tracker = tabletPinches.take(this);
        if (tracker.tracked && tracker.age.isValid() &&
            tracker.age.elapsed() <= qBound(120, QSettings().value(
                QStringLiteral("TabletUi/pinchMs"), 250).toInt(), 500) &&
            tracker.startDistance - tracker.smallestDistance > 82 &&
            tracker.smallestDistance < tracker.startDistance * 0.51 &&
            tracker.smallestDistance < 78) {
            QTimer::singleShot(0, this, []() {
                if (!KisPart::exists()) return;
                KisMainWindow *window = KisPart::instance()->currentMainwindow();
                if (!window) return;
                QAction *rotation = window->actionCollection()->action(
                    QStringLiteral("reset_canvas_rotation"));
                QAction *fit = window->actionCollection()->action(QStringLiteral("zoom_to_fit"));
                if (rotation) rotation->trigger();
                if (fit) fit->trigger();
            });
        }
#endif
        retval = d->matcher.touchEndEvent(touchEvent);
        if (d->touchStrokeStarted) {""")

    # Direct stylus sampling on reference images, regardless of drawing tool.
    # No brush stroke is allowed to start for the consumed tablet sequence.
    path = "libs/ui/input/kis_input_manager.cpp"
    change(path, "#include <QHash>\n", """#include <QHash>
#include <QSet>
#include <KisDocument.h>
#include <KisReferenceImagesLayer.h>
#include <KisReferenceImagesDecoration.h>
#include <KoColor.h>
#include <kis_coordinates_converter.h>
""")
    change(path, """QHash<KisInputManager*, TabletPinchTracking> tabletPinches;
}""",
           """QHash<KisInputManager*, TabletPinchTracking> tabletPinches;
QSet<KisInputManager*> tabletReferencePenPresses;
}""")
    change(path, """KisInputManager::~KisInputManager()
{
#ifdef Q_OS_ANDROID
    tabletPinches.remove(this);
#endif""",
           """KisInputManager::~KisInputManager()
{
#ifdef Q_OS_ANDROID
    tabletPinches.remove(this);
    tabletReferencePenPresses.remove(this);
#endif""")
    change(path, """    case QEvent::TabletPress: {
        d->debugEvent<QTabletEvent, false>(event);
        QTabletEvent *tabletEvent = static_cast<QTabletEvent*>(event);

        {""",
           """    case QEvent::TabletPress: {
        d->debugEvent<QTabletEvent, false>(event);
        QTabletEvent *tabletEvent = static_cast<QTabletEvent*>(event);
#ifdef Q_OS_ANDROID
        if (tabletEvent->button() == Qt::LeftButton &&
            QSettings().value(QStringLiteral("TabletUi/referencePenSample"), true).toBool() &&
            KisPart::exists()) {
            KisMainWindow *window = KisPart::instance()->currentMainwindow();
            KisCanvas2 *canvas = window && window->viewManager()
                ? window->viewManager()->canvasBase() : nullptr;
            if (canvas && window->activeView() &&
                canvas->referenceImagesDecoration() &&
                canvas->referenceImagesDecoration()->visible()) {
                KisReferenceImagesLayerSP layer =
                    window->activeView()->document()->referenceImagesLayer();
                if (layer) {
                    QColor sampled = layer->getPixel(
                        canvas->coordinatesConverter()->widgetToImage(tabletEvent->posF()));
                    if (sampled.isValid() && sampled.alpha() > 0) {
                        KoColor result(sampled, canvas->image()->colorSpace());
                        canvas->resourceManager()->setResource(
                            KoCanvasResource::ForegroundColor, result);
                        tabletReferencePenPresses.insert(this);
                        d->blockMouseEvents();
                        event->accept();
                        return true;
                    }
                }
            }
        }
#endif

        {""")
    change(path, """    case QEvent::TabletMove: {
        d->debugEvent<QTabletEvent, false>(event);""",
           """    case QEvent::TabletMove: {
#ifdef Q_OS_ANDROID
        if (tabletReferencePenPresses.contains(this)) {
            event->accept();
            return true;
        }
#endif
        d->debugEvent<QTabletEvent, false>(event);""")
    change(path, """    case QEvent::TabletRelease: {
#if defined(Q_OS_MAC) || defined(Q_OS_ANDROID)""",
           """    case QEvent::TabletRelease: {
#ifdef Q_OS_ANDROID
        if (tabletReferencePenPresses.remove(this)) {
            d->allowMouseEvents();
            event->accept();
            return true;
        }
#endif
#if defined(Q_OS_MAC) || defined(Q_OS_ANDROID)""")

    # Replace the v1 popup gallery with an inline page occupying the same
    # area as Recent Images; hide the news column on Android.
    path = "libs/ui/KisWelcomePageWidget.cpp"
    src = modified.get(path, (root / path).read_text(encoding="utf-8"))
    begin_token = "        auto *galleryButton = new QToolButton(this);"
    begin = src.find(begin_token)
    end = src.find("\n#endif", begin)
    if begin < 0 or end < 0 or src.count(begin_token) != 1:
        raise RuntimeError("Could not locate v1 gallery injection")
    gallery_code = fragment("tablet_update/583-widgetright-hide.inc")
    src = src[:begin] + gallery_code + src[end:]
    # New gallery uses std::shared_ptr and QComboBox.
    if "#include <algorithm>\n" not in src:
        raise RuntimeError("Gallery dependencies from v1 not applied")
    src = src.replace("#include <algorithm>\n",
                      "#include <algorithm>\n#include <memory>\n#include <QComboBox>\n", 1)
    # Do not let upstream's asynchronous recent-files refresh re-show Clear.
    old_clear='''    clearRecentFilesLink->setVisible(!modelIsEmpty);'''
    new_clear='''#ifdef Q_OS_ANDROID
    clearRecentFilesLink->hide();
#else
    clearRecentFilesLink->setVisible(!modelIsEmpty);
#endif'''
    if src.count(old_clear)!=1:
        raise RuntimeError("Recent files Clear control anchor mismatch")
    src=src.replace(old_clear,new_clear)
    modified[path] = src

    for path, contents in modified.items():
        (root / path).write_text(contents, encoding="utf-8")
        print(f"tablet UI patch: {path}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: apply_tablet_update.py /path/to/krita")
    main(Path(sys.argv[1]))
