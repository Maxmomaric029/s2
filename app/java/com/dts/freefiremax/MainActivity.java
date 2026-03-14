package com.dts.freefireth;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.widget.Toast;

public class MainActivity extends Activity {

    private static final int REQUEST_OVERLAY = 1001;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (!Settings.canDrawOverlays(this)) {
            // Pedir permiso primero
            startActivityForResult(new Intent(
                Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                Uri.parse("package:" + getPackageName())
            ), REQUEST_OVERLAY);
        } else {
            launchAndWait();
        }
    }

    @Override
    protected void onActivityResult(int req, int res, Intent data) {
        if (req == REQUEST_OVERLAY) {
            if (Settings.canDrawOverlays(this)) {
                launchAndWait();
            } else {
                Toast.makeText(this, "Permiso overlay requerido.", Toast.LENGTH_LONG).show();
                finish();
            }
        }
    }

    private void launchAndWait() {
        // 1. Abrir Free Fire primero
        try {
            Intent ff = getPackageManager()
                .getLaunchIntentForPackage("com.dts.freefireth");
            if (ff != null) {
                ff.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                startActivity(ff);
            }
        } catch (Exception ignored) {}

        // 2. Esperar 10 segundos y recién ahí arrancar el overlay
        new Handler(Looper.getMainLooper()).postDelayed(() -> {
            startService(new Intent(this, OverlayService.class));
            finish(); // cerrar la Activity, el Service sigue vivo
        }, 10_000);
    }
}
