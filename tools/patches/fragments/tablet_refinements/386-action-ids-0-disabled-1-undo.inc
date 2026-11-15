// Action IDs: 0 disabled, 1 undo, 2 redo, 3 eraser, 4 pick once,
 // 5 increase brush size, 6 decrease brush size.
extern "C" JNIEXPORT jint JNICALL
Java_org_krita_android_JNIWrappers_volumeKeyAction(JNIEnv*, jobject, jboolean up, jboolean longPress)
{
    QSettings s;
    if (longPress && !s.value(QStringLiteral("TabletUi/volumeLongEnabled"), true).toBool()) return 0;
    if (!longPress && !s.value(QStringLiteral("TabletUi/volumeShortEnabled"), true).toBool()) return 0;
    const QString key = up
       ? (longPress ? QStringLiteral("TabletUi/volumeUpLong") : QStringLiteral("TabletUi/volumeUpShort"))
       : (longPress ? QStringLiteral("TabletUi/volumeDownLong") : QStringLiteral("TabletUi/volumeDownShort"));
    const int fallback = up ? (longPress ? 3 : 2) : (longPress ? 4 : 1);
    return qBound(0, s.value(key, fallback).toInt(), 6);
}

extern "C" JNIEXPORT jint JNICALL
Java_org_krita_android_JNIWrappers_volumeLongPressMs(JNIEnv*, jobject)
{
    return qBound(80, QSettings().value(QStringLiteral("TabletUi/volumeLongMs"), 200).toInt(), 1000);
}

extern "C" JNIEXPORT void JNICALL
Java_org_krita_android_JNIWrappers_volumeExecute(JNIEnv*, jobject, jint code)
{
    if (code <= 0 || code > 6 || !KisPart::exists()) return;
    KisMainWindow *window = KisPart::instance()->currentMainwindow();
    if (!window) return;
    QMetaObject::invokeMethod(window, [window, code]() {
        if (code == 1 || code == 2) {
            QMetaObject::invokeMethod(window, code == 1 ? "undo" : "redo",
                                      Qt::DirectConnection);
            return;
        }
        QString name;
        if (code == 3) name = QStringLiteral("erase_action");
        if (code == 4) name = QStringLiteral("tablet_pick_once");
        if (code == 5) name = QStringLiteral("increase_brush_size");
        if (code == 6) name = QStringLiteral("decrease_brush_size");
        if (QAction *action = window->actionCollection()->action(name))
            action->trigger();
    }, Qt::QueuedConnection);
}

#define MAIN_EXPORT __attribute__ ((visibility ("default")))
#define MAIN_FN main