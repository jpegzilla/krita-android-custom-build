    public static long studioCutoutInsets() {
        MainActivity activity = tabletPermissionActivity;
        if (activity == null || Build.VERSION.SDK_INT < 28) return 0;
        android.view.WindowInsets insets = activity.getWindow().getDecorView().getRootWindowInsets();
        android.view.DisplayCutout cutout = insets == null ? null : insets.getDisplayCutout();
        if (cutout == null) return 0;
        return ((long) cutout.getSafeInsetLeft() << 48)
            | ((long) cutout.getSafeInsetTop() << 32)
            | ((long) cutout.getSafeInsetRight() << 16)
            | cutout.getSafeInsetBottom();
    }

    private boolean protectCanvasBack = false;
    private Object canvasBackCallback;

    public static void setCanvasBackProtection(boolean enabled) {
        MainActivity activity = tabletPermissionActivity;
        if (activity == null) return;
        activity.runOnUiThread(() -> {
            if (activity.protectCanvasBack == enabled) return;
            activity.protectCanvasBack = enabled;
            if (Build.VERSION.SDK_INT >= 33) activity.updateCanvasBackCallback(enabled);
        });
    }

    @RequiresApi(api = 33)
    private void updateCanvasBackCallback(boolean enabled) {
        if (enabled) {
            android.window.OnBackInvokedCallback callback = () -> {};
            canvasBackCallback = callback;
            getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
                android.window.OnBackInvokedDispatcher.PRIORITY_DEFAULT, callback);
        } else if (canvasBackCallback != null) {
            getOnBackInvokedDispatcher().unregisterOnBackInvokedCallback(
                (android.window.OnBackInvokedCallback) canvasBackCallback);
            canvasBackCallback = null;
        }
    }

    @Override
    public void onBackPressed() {
        if (!protectCanvasBack) super.onBackPressed();
    }

    public boolean isInFullScreen() {