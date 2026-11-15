    // Pressure proxy diagnostics use Android's raw contact dimensions
    // and historical samples before Qt may coalesce touch updates.
    // Keep the underlying Qt touchscreen behavior exactly unchanged.
    private boolean fingerRawBridgeAvailable = true;
    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (haveLibsLoaded && fingerRawBridgeAvailable && event != null) {
            final int action = event.getActionMasked();
            // Android reuses pointer IDs from one gesture to the next.
            // Never allow a new or interrupted gesture to read contact
            // sizes from a previous finger, or from a multi-finger pan.
            if (action == MotionEvent.ACTION_DOWN ||
                    action == MotionEvent.ACTION_POINTER_DOWN ||
                    action == MotionEvent.ACTION_CANCEL) {
                try {
                    JNIWrappers.fingerRawGestureReset();
                } catch (UnsatisfiedLinkError unavailable) {
                    fingerRawBridgeAvailable = false;
                }
            }
            if (fingerRawBridgeAvailable &&
                    event.getPointerCount() == 1 &&
                    event.getToolType(0) == MotionEvent.TOOL_TYPE_FINGER &&
                    (action == MotionEvent.ACTION_DOWN ||
                     action == MotionEvent.ACTION_MOVE ||
                     action == MotionEvent.ACTION_UP)) {
                // User must opt in: no raw size samples are recorded
                // while the experimental finger-pressure toggle is off.
                // A cached JNI flag avoids settings reads per touch frame.
                boolean capture = false;
                try {
                    capture = JNIWrappers.fingerRawCaptureEnabled(
                        event.getEventTime());
                } catch (UnsatisfiedLinkError unavailable) {
                    fingerRawBridgeAvailable = false;
                }
                if (capture) {
                    final int id = event.getPointerId(0);
                    final float density = getResources()
                        .getDisplayMetrics().density;
                    try {
                        final int history = event.getHistorySize();
                    // Keep the most recent historical samples. The native
                    // buffer itself is bounded and records no positions.
                    for (int i=Math.max(0, history-8); i<history; ++i) {
                        JNIWrappers.fingerRawContactSample(id,
                            event.getHistoricalEventTime(i),
                            event.getHistoricalTouchMajor(0,i),
                            event.getHistoricalTouchMinor(0,i),
                            density,true);
                    }
                    JNIWrappers.fingerRawContactSample(id,
                        event.getEventTime(), event.getTouchMajor(0),
                        event.getTouchMinor(0), density,false);
                    } catch (UnsatisfiedLinkError unavailable) {
                        // Optional native symbol missing: do not break Qt
                        // painting, document dialogs or normal touch input.
                        fingerRawBridgeAvailable = false;
                    }
                }
            }
        }
        return super.dispatchTouchEvent(event);
    }

    private static final String TAG = "krita.MainActivity";