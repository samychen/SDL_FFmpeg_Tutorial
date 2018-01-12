package com.samychen.gracefulwrapper.liveplayer;

import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class ANativeWindowPlayerActivity extends AppCompatActivity implements SurfaceHolder.Callback{

    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;
    private String filePath;
    static{
        System.loadLibrary("SDL2");
        System.loadLibrary("SDL2_image");
        System.loadLibrary("SDL2_mixer");
        System.loadLibrary("SDL2main");
    }
    public native int play(String url, Object surface);
    public native int decode(String inputurl, String outputurl);
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_anative_window_player);
        surfaceView = (SurfaceView) findViewById(R.id.surface);
        Intent intent = getIntent();
        String path = intent.getStringExtra("path");
        filePath = getInnerSDCardPath() + "/"+path;
//        filePath = "http://bepvideo.baoerpai.com/00D1D5465F187F389C33DC5901307461";
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
        //decode(filePath, getInnerSDCardPath() + "/decode.yuv");
    }

    public String getInnerSDCardPath() {
        return Environment.getExternalStorageDirectory().getPath();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                play(filePath, surfaceHolder.getSurface());
            }
        }).start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }
}
