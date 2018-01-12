package com.samychen.gracefulwrapper.liveplayer;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.widget.EditText;

import org.libsdl.app.SDLActivity;

public class MainActivity extends AppCompatActivity {
    static{
//        System.loadLibrary("avcodec-57");
//        System.loadLibrary("avfilter-6");
//        System.loadLibrary("avformat-57");
//        System.loadLibrary("avresample-3");
//        System.loadLibrary("avutil-55");
//        System.loadLibrary("crypto");
//        System.loadLibrary("postproc-54");
//        System.loadLibrary("swresample-2");
//        System.loadLibrary("swscale-4");
//        System.loadLibrary("x264");
//        System.loadLibrary("ssl");
//        System.loadLibrary("rtmp");
//        System.loadLibrary("SDL2");
//        System.loadLibrary("SDL2main");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        DisplayMetrics metrics=new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);
        int widthPixels=metrics.widthPixels;
        int heightPixels=metrics.heightPixels;
        Log.d("main", "onCreate: "+widthPixels+"x"+heightPixels);
    }

        public void clickSDLTest(View v){
            startActivity(new Intent(this, SDLActivity.class));
        }
        public void clickSDLPlayer(View v){
            EditText url = (EditText) findViewById(R.id.url);
            Intent intent = new Intent(this, Main2Activity.class);
            intent.putExtra("url",url.getText().toString());
            startActivity(intent);
        }
    public void clickStreamPush(View v){
        startActivity(new Intent(this, StreamActivity.class));
    }
    public void clickNative(View v){
        EditText url = (EditText) findViewById(R.id.path);
        startActivity(new Intent(this,ANativeWindowPlayerActivity.class).
                putExtra("path",url.getText().toString()));
    }
    public void clickOpenSL(View v){
        startActivity(new Intent(this,OpenSLESActivity.class));
    }
//    public native void Init();
//    public native int InitVideo();
//    public native int InitAudio();
}
