package com.dts.freefireth;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.widget.Toast;

public class MainActivity extends Activity {
    private static final int REQUEST_OVERLAY = 1001;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (!Settings.canDrawOverlays(this)) {
            startActivityForResult(new Intent(
                Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                Uri.parse("package:" + getPackageName())
            ), REQUEST_OVERLAY);
        } else {
            startOverlay();
        }
    }

    @Override
    protected void onActivityResult(int req, int res, Intent data) {
        if (req == REQUEST_OVERLAY) {
            if (Settings.canDrawOverlays(this)) startOverlay();
            else {
                Toast.makeText(this, "Permiso overlay requerido.", Toast.LENGTH_LONG).show();
                finish();
            }
        }
    }

    private void startOverlay() {
        startService(new Intent(this, OverlayService.class));
        moveTaskToBack(true);
    }
}
