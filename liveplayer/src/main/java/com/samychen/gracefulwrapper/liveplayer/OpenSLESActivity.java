package com.samychen.gracefulwrapper.liveplayer;

import android.Manifest;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.SurfaceView;
import android.view.View;
import android.widget.EditText;

public class OpenSLESActivity extends AppCompatActivity {

    SurfaceView surfaceView;
    DNPlayer dnPlayer;
    EditText src;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_open_sles);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 100);
        }
        surfaceView = (SurfaceView) findViewById(R.id.surfaceView);
        src = (EditText) findViewById(R.id.src);
        dnPlayer = new DNPlayer();
        //支持后台播放....
        dnPlayer.setDisplay(surfaceView);
    }
    public void play(View view) {
        String edittxt = src.getText().toString().trim();
        if (edittxt.startsWith("rtmp")||edittxt.startsWith("rtsp")){
            dnPlayer.play(edittxt);
        } else {
            String folderurl= Environment.getExternalStorageDirectory().getPath();
            String urltext_input=src.getText().toString();
            String inputurl=folderurl+"/"+urltext_input;
            dnPlayer.play(inputurl);
        }
//        MediaPlayer mediaPlayer = new MediaPlayer();
//        try {
//            mediaPlayer.setDataSource("/sdcard/a.flv");
//            mediaPlayer.setDisplay(surfaceView.getHolder());
//            mediaPlayer.prepare();
//            mediaPlayer.start();
//        } catch (IOException e) {
//            e.printStackTrace();
//        }
    }
    public void stop(View view) {
        dnPlayer.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        dnPlayer.release();
    }
}
