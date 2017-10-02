package com.tz.dream.sdl.ffmpeg.demo;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

import org.libsdl.app.SDLActivity;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    public void clickSDLTest(View v){
        startActivity(new Intent(this, SDLActivity.class));
    }

    public void clickSDLPlayer(View v){
        startActivity(new Intent(this, SDLActivity.class));
    }
    public void clickPcmVoice(View v){
        startActivity(new Intent(this, SDLActivity.class));
    }
}
