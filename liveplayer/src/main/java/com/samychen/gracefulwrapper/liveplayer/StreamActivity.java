package com.samychen.gracefulwrapper.liveplayer;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

public class StreamActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_stream);
        Button startButton = (Button) this.findViewById(R.id.button_start);
        final EditText urlEdittext_input= (EditText) this.findViewById(R.id.input_url);
        final EditText urlEdittext_output= (EditText) this.findViewById(R.id.output_url);
        startButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View arg0){
                String folderurl= Environment.getExternalStorageDirectory().getPath();
                String urltext_input=urlEdittext_input.getText().toString();
                String inputurl=folderurl+"/"+urltext_input;
                String outputurl=urlEdittext_output.getText().toString();
                Log.e("inputurl",inputurl);
                Log.e("outputurl",outputurl);
                String info="";
                stream(inputurl,outputurl);
                Log.e("Info",info);
            }
        });
        Button stopButton = (Button) findViewById(R.id.button_stop);
        stopButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                stop();
            }
        });
    }
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    //JNI
    static native void stream(String inputurl, String outputurl);
    static native void stop();

    static{
        System.loadLibrary("SDL2");
        System.loadLibrary("SDL2_image");
        System.loadLibrary("SDL2_mixer");
        System.loadLibrary("SDL2main");
    }
}
