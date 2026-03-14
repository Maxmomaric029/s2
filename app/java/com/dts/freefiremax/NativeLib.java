package com.dts.freefireth;

public class NativeLib {
    static { System.loadLibrary("modmenu"); }
    public static native void toggleFeature(int featureId, boolean value);
    public static native void setAimbotParam(int paramId, float value);
    public static native void setScreenSize(int width, int height);
}
