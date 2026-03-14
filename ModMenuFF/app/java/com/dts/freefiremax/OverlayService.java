package com.dts.freefiremax;

import android.app.Service;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.os.IBinder;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;

public class OverlayService extends Service {

    private WindowManager wm;
    private View          fabBtn;
    private View          menuPanel;
    private EspRenderer   espView;
    private boolean       menuOpen = false;

    // Estados
    private boolean bAimbot    = false;
    private boolean bEsp       = false;
    private boolean bSilent    = false;
    private boolean bNoRecoil  = false;
    private float   fSmoothing = 5f;
    private float   fFov       = 180f;

    @Override public IBinder onBind(Intent i) { return null; }

    @Override
    public void onCreate() {
        super.onCreate();
        wm = (WindowManager) getSystemService(WINDOW_SERVICE);

        // Pasar resolución real a C++
        DisplayMetrics dm = new DisplayMetrics();
        wm.getDefaultDisplay().getRealMetrics(dm);
        NativeLib.setScreenSize(dm.widthPixels, dm.heightPixels);

        buildEspOverlay();
        buildFab();
        buildMenu();
    }

    // ── ESP SurfaceView (capa más baja, no intercepta toques) ─────────────────
    private void buildEspOverlay() {
        espView = new EspRenderer(this);
        WindowManager.LayoutParams p = new WindowManager.LayoutParams(
            WindowManager.LayoutParams.MATCH_PARENT,
            WindowManager.LayoutParams.MATCH_PARENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |
            WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE |
            WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
            PixelFormat.TRANSPARENT
        );
        wm.addView(espView, p);
    }

    // ── Botón flotante ────────────────────────────────────────────────────────
    private void buildFab() {
        TextView btn = new TextView(this);
        btn.setText("FF");
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(11f);
        btn.setGravity(android.view.Gravity.CENTER);
        btn.setBackgroundColor(Color.parseColor("#CC0A0A1A"));
        btn.setPadding(18, 10, 18, 10);
        fabBtn = btn;

        WindowManager.LayoutParams p = new WindowManager.LayoutParams(
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        );
        p.gravity = Gravity.TOP | Gravity.START;
        p.x = 8; p.y = 220;

        makeDraggable(fabBtn, p, () -> toggleMenu());
        wm.addView(fabBtn, p);
    }

    // ── Panel del menú ────────────────────────────────────────────────────────
    private void buildMenu() {
        ScrollView scroll = new ScrollView(this);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(Color.parseColor("#F0000010"));
        root.setPadding(20, 20, 20, 20);

        // Título
        TextView title = new TextView(this);
        title.setText("FREE FIRE MAX  |  MOD MENU");
        title.setTextColor(Color.parseColor("#FF4C8C"));
        title.setTextSize(13f);
        title.setPadding(0, 0, 0, 14);
        root.addView(title);

        // Sección: Combat
        root.addView(sectionLabel("COMBAT"));
        root.addView(makeSwitch("Aimbot", bAimbot, v -> {
            bAimbot = v; NativeLib.toggleFeature(0, v);
        }));
        root.addView(makeSwitch("Silent Aim", bSilent, v -> {
            bSilent = v; NativeLib.toggleFeature(2, v);
        }));
        root.addView(makeSwitch("No Recoil", bNoRecoil, v -> {
            bNoRecoil = v; NativeLib.toggleFeature(3, v);
        }));

        // Sección: Visual
        root.addView(sectionLabel("VISUAL"));
        root.addView(makeSwitch("ESP  (caja + distancia)", bEsp, v -> {
            bEsp = v; NativeLib.toggleFeature(1, v);
        }));

        // Sección: Aimbot config
        root.addView(sectionLabel("AIMBOT CONFIG"));

        final TextView lblSmooth = makeLabel("Smoothing: " + fSmoothing);
        root.addView(lblSmooth);
        SeekBar sbSmooth = makeSeekBar(100, (int)(fSmoothing * 10), p -> {
            fSmoothing = Math.max(1f, p / 10f);
            lblSmooth.setText("Smoothing: " + fSmoothing);
            NativeLib.setAimbotParam(1, fSmoothing);
        });
        root.addView(sbSmooth);

        final TextView lblFov = makeLabel("FOV: " + (int)fFov);
        root.addView(lblFov);
        SeekBar sbFov = makeSeekBar(360, (int)fFov, p -> {
            fFov = Math.max(10f, p);
            lblFov.setText("FOV: " + (int)fFov);
            NativeLib.setAimbotParam(0, fFov);
        });
        root.addView(sbFov);

        // Cerrar
        root.addView(divider());
        TextView close = new TextView(this);
        close.setText("[ cerrar ]");
        close.setTextColor(Color.parseColor("#FF4C8C"));
        close.setTextSize(12f);
        close.setPadding(0, 8, 0, 0);
        close.setOnClickListener(x -> toggleMenu());
        root.addView(close);

        scroll.addView(root);
        menuPanel = scroll;
        menuPanel.setVisibility(View.GONE);

        WindowManager.LayoutParams p = new WindowManager.LayoutParams(
            dpToPx(240),
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        );
        p.gravity = Gravity.TOP | Gravity.START;
        p.x = 8; p.y = 60;
        wm.addView(menuPanel, p);
    }

    // ── Helpers UI ────────────────────────────────────────────────────────────
    private interface OnCheck { void on(boolean v); }
    private interface OnProgress { void on(int p); }

    private Switch makeSwitch(String label, boolean init, OnCheck cb) {
        Switch sw = new Switch(this);
        sw.setText(label); sw.setTextColor(Color.WHITE);
        sw.setChecked(init);
        sw.setOnCheckedChangeListener((v, c) -> cb.on(c));
        sw.setPadding(0, 8, 0, 8);
        return sw;
    }

    private TextView sectionLabel(String t) {
        TextView tv = new TextView(this);
        tv.setText(t);
        tv.setTextColor(Color.parseColor("#888888"));
        tv.setTextSize(10f);
        tv.setPadding(0, 14, 0, 4);
        return tv;
    }

    private TextView makeLabel(String t) {
        TextView tv = new TextView(this);
        tv.setText(t); tv.setTextColor(Color.parseColor("#AAAAAA"));
        tv.setTextSize(11f); tv.setPadding(0, 6, 0, 2);
        return tv;
    }

    private SeekBar makeSeekBar(int max, int progress, OnProgress cb) {
        SeekBar sb = new SeekBar(this);
        sb.setMax(max); sb.setProgress(progress);
        sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar s, int p, boolean u) { cb.on(p); }
            public void onStartTrackingTouch(SeekBar s) {}
            public void onStopTrackingTouch(SeekBar s) {}
        });
        return sb;
    }

    private View divider() {
        View v = new View(this);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, 1);
        lp.setMargins(0, 10, 0, 10);
        v.setLayoutParams(lp);
        v.setBackgroundColor(Color.parseColor("#33FFFFFF"));
        return v;
    }

    private int dpToPx(int dp) {
        return (int)(dp * getResources().getDisplayMetrics().density);
    }

    private void toggleMenu() {
        menuOpen = !menuOpen;
        menuPanel.setVisibility(menuOpen ? View.VISIBLE : View.GONE);
    }

    // ── Arrastrar con detección de click ──────────────────────────────────────
    private void makeDraggable(View v, WindowManager.LayoutParams p, Runnable onClick) {
        v.setOnTouchListener(new View.OnTouchListener() {
            int lx, ly, sx, sy;
            public boolean onTouch(View vv, MotionEvent e) {
                switch (e.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        lx = (int)e.getRawX(); ly = (int)e.getRawY();
                        sx = lx; sy = ly; return true;
                    case MotionEvent.ACTION_MOVE:
                        p.x += (int)e.getRawX() - lx;
                        p.y += (int)e.getRawY() - ly;
                        lx = (int)e.getRawX(); ly = (int)e.getRawY();
                        wm.updateViewLayout(v, p); return true;
                    case MotionEvent.ACTION_UP:
                        if (Math.abs(e.getRawX()-sx)<12 && Math.abs(e.getRawY()-sy)<12)
                            onClick.run();
                        return true;
                }
                return false;
            }
        });
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (fabBtn   != null) wm.removeView(fabBtn);
        if (menuPanel!= null) wm.removeView(menuPanel);
        if (espView  != null) wm.removeView(espView);
    }
}
