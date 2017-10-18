package com.tz.dream.sdl.ffmpeg.demo;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

import org.libsdl.app.SDLActivity;

public class MainActivity extends AppCompatActivity {
    static{
//        System.loadLibrary("avcodec-57");
//        System.loadLibrary("avfilter-6");
//        System.loadLibrary("avformat-57");
//        System.loadLibrary("avutil-55");
//        System.loadLibrary("swresample-2");
//        System.loadLibrary("swscale-4");
        System.loadLibrary("SDL2");
        System.loadLibrary("SDL2main");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
       
    }

    public void clickSDLTest(View v){
        playrtmp();
    }
    public native void playrtmp();
    public void clickSDLPlayer(View v){
        startActivity(new Intent(this, SDLActivity.class));
    }
    public void clickPcmVoice(View v){
        startActivity(new Intent(this, SDLActivity.class));
    }
}
