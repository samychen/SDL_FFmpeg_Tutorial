package com.samychen.gracefulwrapper.liveplayer;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * Created by xiang on 2017/9/22.
 */

public class DNPlayer implements SurfaceHolder.Callback {

    static {
        System.loadLibrary("SDL2main");
    }

    private SurfaceView display;

    private native void native_play(String path);

    private native void native_set_display(Surface surface);

    private native void native_stop();

    private native void native_release();


    public void play(String path) {
        if (null == display) {
            return;
        }
        native_play(path);
    }

    public void stop() {
        native_stop();
    }

    public void release() {
        stop();
        native_release();
    }


    public void setDisplay(SurfaceView display) {
        if (null != this.display) {
            this.display.getHolder().removeCallback(this);
        }
        this.display = display;
        native_set_display(display.getHolder().getSurface());
        this.display.getHolder().addCallback(this);
    }




    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        native_set_display(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }
}
