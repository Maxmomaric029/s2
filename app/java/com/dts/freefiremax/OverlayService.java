package com.dts.freefireth;

import android.app.Service;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Typeface;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
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
    private boolean       menuOpen  = false;
    private boolean       espAdded  = false;
    private final Handler handler   = new Handler(Looper.getMainLooper());

    // Estados — todos desactivados por defecto
    private boolean bAimbot    = false;
    private boolean bEsp       = false;
    private boolean bSilent    = false;
    private boolean bNoRecoil  = false;
    private float   fSmoothing = 5f;
    private float   fFov       = 180f;

    // Para el delay de 5 segundos
    private Runnable pendingToggle = null;

    @Override public IBinder onBind(Intent i) { return null; }

    @Override
    public void onCreate() {
        super.onCreate();
        wm = (WindowManager) getSystemService(WINDOW_SERVICE);
        DisplayMetrics dm = new DisplayMetrics();
        wm.getDefaultDisplay().getRealMetrics(dm);
        NativeLib.setScreenSize(dm.widthPixels, dm.heightPixels);
        buildFab();
        buildMenu();
    }

    // ── Botón flotante pequeño con indicador de estado ────────────────────────
    private void buildFab() {
        TextView btn = new TextView(this);
        btn.setText("●");
        btn.setTextColor(Color.parseColor("#FF4C8C"));
        btn.setTextSize(18f);
        btn.setGravity(android.view.Gravity.CENTER);
        btn.setBackgroundColor(Color.parseColor("#CC000010"));
        btn.setPadding(14, 8, 14, 8);
        fabBtn = btn;

        WindowManager.LayoutParams p = new WindowManager.LayoutParams(
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        );
        p.gravity = Gravity.TOP | Gravity.START;
        p.x = 8; p.y = 180;

        makeDraggable(fabBtn, p, this::toggleMenu);
        wm.addView(fabBtn, p);
    }

    // Actualizar color del botón según si hay algún hack activo
    private void updateFabState() {
        boolean anyActive = bAimbot || bEsp || bSilent || bNoRecoil;
        ((TextView) fabBtn).setTextColor(
            anyActive ? Color.parseColor("#44FF44") : Color.parseColor("#FF4C8C")
        );
    }

    // ── Menú minimalista ─────────────────────────────────────────────────────
    private void buildMenu() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(Color.parseColor("#F0000010"));
        root.setPadding(18, 18, 18, 18);

        // Título
        TextView title = new TextView(this);
        title.setText("9B MOD");
        title.setTextColor(Color.parseColor("#FF4C8C"));
        title.setTextSize(14f);
        title.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
        title.setPadding(0, 0, 0, 14);
        root.addView(title);

        root.addView(divider());

        // Switches — todos desactivados por defecto, delay de 5s al activar
        root.addView(buildSwitch("AIMBOT", false, on -> scheduleToggle(0, on)));
        root.addView(buildSwitch("SILENT AIM", false, on -> scheduleToggle(2, on)));
        root.addView(buildSwitch("NO RECOIL", false, on -> scheduleToggle(3, on)));
        root.addView(buildSwitch("ESP", false, on -> {
            scheduleToggle(1, on);
            if (on) handler.postDelayed(this::attachEsp, 5000);
            else    detachEsp();
        }));

        root.addView(divider());

        // Smoothing
        TextView lblS = label("SMOOTH  " + (int)fSmoothing);
        root.addView(lblS);
        SeekBar sbS = seekbar(100, (int)(fSmoothing*10), p -> {
            fSmoothing = Math.max(1f, p / 10f);
            lblS.setText("SMOOTH  " + (int)fSmoothing);
            NativeLib.setAimbotParam(1, fSmoothing);
        });
        root.addView(sbS);

        root.addView(divider());

        // Cerrar
        TextView close = new TextView(this);
        close.setText("[ X ]");
        close.setTextColor(Color.parseColor("#555555"));
        close.setTextSize(11f);
        close.setTypeface(Typeface.MONOSPACE);
        close.setOnClickListener(x -> toggleMenu());
        root.addView(close);

        menuPanel = root;
        menuPanel.setVisibility(View.GONE);

        WindowManager.LayoutParams p = new WindowManager.LayoutParams(
            dpToPx(190),
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        );
        p.gravity = Gravity.TOP | Gravity.START;
        p.x = 8; p.y = 50;
        wm.addView(menuPanel, p);
    }

    // Activar un hack con delay de 5 segundos
    private void scheduleToggle(int id, boolean on) {
        handler.postDelayed(() -> {
            NativeLib.toggleFeature(id, on);
            // Actualizar estado interno
            switch(id) {
                case 0: bAimbot   = on; break;
                case 1: bEsp      = on; break;
                case 2: bSilent   = on; break;
                case 3: bNoRecoil = on; break;
            }
            updateFabState();
        }, on ? 5000 : 0); // 5s al activar, instantáneo al desactivar
    }

    // ── ESP overlay ───────────────────────────────────────────────────────────
    private void attachEsp() {
        if (espAdded) return;
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
        espAdded = true;
    }

    private void detachEsp() {
        if (!espAdded || espView == null) return;
        wm.removeView(espView);
        espView = null; espAdded = false;
    }

    // ── Helpers ───────────────────────────────────────────────────────────────
    private interface OnToggle { void on(boolean v); }
    private interface OnProg   { void on(int p);     }

    private Switch buildSwitch(String label, boolean init, OnToggle cb) {
        Switch sw = new Switch(this);
        sw.setText(label);
        sw.setTextColor(Color.parseColor("#CCCCCC"));
        sw.setTypeface(Typeface.MONOSPACE);
        sw.setTextSize(11f);
        sw.setChecked(init);
        sw.setOnCheckedChangeListener((v, c) -> cb.on(c));
        sw.setPadding(0, 10, 0, 10);
        return sw;
    }

    private TextView label(String t) {
        TextView tv = new TextView(this);
        tv.setText(t);
        tv.setTextColor(Color.parseColor("#666666"));
        tv.setTypeface(Typeface.MONOSPACE);
        tv.setTextSize(10f);
        tv.setPadding(0, 6, 0, 2);
        return tv;
    }

    private SeekBar seekbar(int max, int prog, OnProg cb) {
        SeekBar sb = new SeekBar(this);
        sb.setMax(max); sb.setProgress(prog);
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
        lp.setMargins(0, 8, 0, 8);
        v.setLayoutParams(lp);
        v.setBackgroundColor(Color.parseColor("#22FFFFFF"));
        return v;
    }

    private int dpToPx(int dp) {
        return (int)(dp * getResources().getDisplayMetrics().density);
    }

    private void toggleMenu() {
        menuOpen = !menuOpen;
        menuPanel.setVisibility(menuOpen ? View.VISIBLE : View.GONE);
    }

    private void makeDraggable(View v, WindowManager.LayoutParams p, Runnable onClick) {
        v.setOnTouchListener(new View.OnTouchListener() {
            int lx, ly, sx, sy;
            public boolean onTouch(View vv, MotionEvent e) {
                switch (e.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        lx=(int)e.getRawX(); ly=(int)e.getRawY();
                        sx=lx; sy=ly; return true;
                    case MotionEvent.ACTION_MOVE:
                        p.x+=(int)e.getRawX()-lx; p.y+=(int)e.getRawY()-ly;
                        lx=(int)e.getRawX(); ly=(int)e.getRawY();
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
        detachEsp();
        if (fabBtn    != null) wm.removeView(fabBtn);
        if (menuPanel != null) wm.removeView(menuPanel);
    }
}
