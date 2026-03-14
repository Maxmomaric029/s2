package com.dts.freefireth;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class EspRenderer extends SurfaceView implements SurfaceHolder.Callback {

    // 9 floats por enemigo: headX, headY, footX, footY, hipX, hipY, dist, hp, visible
    private static final int STRIDE = 9;
    private static volatile float[] sData = new float[0];

    private final Paint pBoxVis  = stroke(Color.parseColor("#FF4C8C"), 2f);
    private final Paint pBoxHide = stroke(Color.parseColor("#FF8800"), 2f);
    private final Paint pHpG     = stroke(Color.parseColor("#44FF44"), 3f);
    private final Paint pHpR     = stroke(Color.parseColor("#FF2222"), 3f);
    private final Paint pText    = text(Color.WHITE,        20f);
    private final Paint pShadow  = text(Color.parseColor("#AA000000"), 20f);
    private final Paint pLine    = stroke(Color.parseColor("#88FFFFFF"), 1f);
    private final Paint pFov;

    private RenderThread thread;

    public EspRenderer(Context ctx) {
        super(ctx);
        setZOrderOnTop(true);
        getHolder().setFormat(android.graphics.PixelFormat.TRANSPARENT);
        getHolder().addCallback(this);

        // FOV circle
        pFov = new Paint(Paint.ANTI_ALIAS_FLAG);
        pFov.setColor(Color.parseColor("#55FFFFFF"));
        pFov.setStyle(Paint.Style.STROKE);
        pFov.setStrokeWidth(1.5f);
    }

    // Llamado por C++ (JNI) cada frame
    public static void updateEspData(float[] data) { sData = data; }

    @Override public void surfaceCreated(SurfaceHolder h) {
        thread = new RenderThread(h); thread.running = true; thread.start();
    }
    @Override public void surfaceChanged(SurfaceHolder h, int f, int w, int ht) {}
    @Override public void surfaceDestroyed(SurfaceHolder h) {
        if (thread != null) thread.running = false;
    }

    private class RenderThread extends Thread {
        final SurfaceHolder holder;
        volatile boolean running;
        RenderThread(SurfaceHolder h) { holder = h; }
        @Override public void run() {
            while (running) {
                Canvas c = null;
                try {
                    c = holder.lockCanvas();
                    if (c != null) {
                        c.drawColor(Color.TRANSPARENT, android.graphics.PorterDuff.Mode.CLEAR);
                        drawEsp(c);
                    }
                } finally { if (c != null) holder.unlockCanvasAndPost(c); }
                try { Thread.sleep(16); } catch (InterruptedException ignored) {}
            }
        }
    }

    private void drawEsp(Canvas c) {
        float[] d = sData;
        if (d == null || d.length < STRIDE) return;
        int n = d.length / STRIDE;

        float cx = c.getWidth()  / 2f;
        float cy = c.getHeight() / 2f;

        for (int i = 0; i < n; i++) {
            int b   = i * STRIDE;
            float hx  = d[b],     hy  = d[b+1];
            float fx  = d[b+2],   fy  = d[b+3];
            float dist= d[b+6],   hp  = d[b+7];
            boolean vis = d[b+8] > 0.5f;

            // Solo dibujar si cabeza está en pantalla (z > 0 → screen.z guardado en hipY z)
            if (hx == 0 && hy == 0) continue;

            Paint boxP = vis ? pBoxVis : pBoxHide;
            float h = fy - hy;
            float w = h * 0.38f;

            // Caja ESP
            float left = hx - w/2f, right = hx + w/2f;
            c.drawRect(left, hy, right, fy, boxP);

            // Barra de vida (izquierda)
            float barX = left - 5f;
            float frac = Math.max(0f, Math.min(1f, hp / 100f));
            c.drawLine(barX, fy, barX, fy - h * frac, frac > 0.4f ? pHpG : pHpR);

            // Distancia
            String dTxt = String.format("%.0fm", dist);
            c.drawText(dTxt, hx+1, hy-10, pShadow);
            c.drawText(dTxt, hx,   hy-11, pText);

            // Línea al centro de pantalla (radar 2D)
            c.drawLine(cx, cy, hx, hy, pLine);
        }
    }

    private static Paint stroke(int color, float w) {
        Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        p.setColor(color); p.setStyle(Paint.Style.STROKE); p.setStrokeWidth(w);
        return p;
    }
    private static Paint text(int color, float size) {
        Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        p.setColor(color); p.setTextSize(size);
        p.setTypeface(Typeface.MONOSPACE); return p;
    }
}
