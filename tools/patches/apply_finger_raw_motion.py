#!/usr/bin/env python3
"""Capture Android finger contact major/minor + historical samples for Krita.

Applied AFTER apply_tablet_refinements.py and apply_finger_contact_pressure.py.
Does not intercept or consume touch events; Qt retains the original gesture,
stylus and brush routing. The Java trampoline stores size only, never points.
"""
from pathlib import Path
import sys

from patch_paths import CORE, fragment, replace_once


def main(root):
    root=Path(root).resolve()
    jni=root/"packaging/android/apk/src/org/krita/android/JNIWrappers.java"
    activity=root/"packaging/android/apk/src/org/krita/android/MainActivity.java"
    ko=root/"libs/flake/KoPointerEvent.cpp"
    header=CORE/"finger_raw_bridge.h"
    for path in (jni,activity,ko,header):
        if not path.is_file():
            raise RuntimeError(f"Missing raw finger bridge input: {path}")

    s=jni.read_text(encoding="utf-8")
    s=replace_once(s,"class JNIWrappers {",
        """class JNIWrappers {
    // Called only for single-finger Android MotionEvents. Does not
    // transmit screen coordinates or alter Java/Qt event dispatch.
    public static native void fingerRawContactSample(
        int pointerId, long eventUptimeMs, float majorPixels,
        float minorPixels, float density, boolean historical);
    // Reset the previous stroke's size-only ring buffer on finger-down,
    // pointer-down (multitouch) and cancelled gestures.
    public static native void fingerRawGestureReset();
    public static native boolean fingerRawCaptureEnabled(long eventUptimeMs);""",
        "Java native declaration")
    jni.write_text(s,encoding="utf-8")

    s=activity.read_text(encoding="utf-8")
    s=replace_once(s,
        "    private static final String TAG = \"krita.MainActivity\";",
        fragment("finger_raw_motion/42-pressure-proxy-diagnostics-u.inc"),
        "MainActivity raw touch trampoline")
    activity.write_text(s,encoding="utf-8")

    s=ko.read_text(encoding="utf-8")
    s=replace_once(s,
        '#include "finger_contact_pressure.h"',
        '''#include "finger_contact_pressure.h"
#include "finger_raw_bridge.h"
#include <jni.h>''',
        "KoPointerEvent Android JNI includes")
    s += """
#ifdef Q_OS_ANDROID
// JNI export is in Krita's flake input module. Only contact dimensions,
// pointer IDs and event uptime cross this boundary. Qt remains responsible
// for brush/gesture event consumption. Historical samples are retained in
// a bounded in-memory buffer for diagnostics and event-time matching.
extern "C" JNIEXPORT void JNICALL
Java_org_krita_android_JNIWrappers_fingerRawContactSample(
    JNIEnv*,jclass,jint pointerId,jlong uptimeMs,jfloat major,
    jfloat minor,jfloat density,jboolean historical)
{
    MobileFingerRawBridge::record(static_cast<int>(pointerId),
        static_cast<quint64>(uptimeMs),major,minor,density,
        historical==JNI_TRUE);
}
extern "C" JNIEXPORT void JNICALL
Java_org_krita_android_JNIWrappers_fingerRawGestureReset(JNIEnv*,jclass)
{
    MobileFingerRawBridge::reset();
}
extern "C" JNIEXPORT jboolean JNICALL
Java_org_krita_android_JNIWrappers_fingerRawCaptureEnabled(
    JNIEnv*,jclass,jlong uptimeMs)
{
    return MobileFingerRawBridge::captureEnabled(
        static_cast<quint64>(uptimeMs)) ? JNI_TRUE : JNI_FALSE;
}
#endif
"""
    ko.write_text(s,encoding="utf-8")
    (root/"libs/flake/finger_raw_bridge.h").write_text(
        header.read_text(encoding="utf-8"),encoding="utf-8")
    print("Installed Android MotionEvent major/minor history bridge")


if __name__=="__main__":
    if len(sys.argv)!=2:
        raise SystemExit("Usage: apply_finger_raw_motion.py <krita-source-root>")
    main(sys.argv[1])
