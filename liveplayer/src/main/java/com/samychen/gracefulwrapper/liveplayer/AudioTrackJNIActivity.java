package com.samychen.gracefulwrapper.liveplayer;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.EditText;

public class AudioTrackJNIActivity extends AppCompatActivity {


    static {
        System.loadLibrary("SDL2");
        System.loadLibrary("SDL2main");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_audio_track_jni);
    }
    public void play(View v){
        EditText inputrul = (EditText) findViewById(R.id.input_url);
        EditText output_url = (EditText) findViewById(R.id.output_url);
        String folder = getInnerSDCardPath()+"/";
        //beat.wav high.wav Test.pcm
        play(folder+inputrul.getText().toString().trim(),folder+output_url.getText().toString().trim());
    }
    public String getInnerSDCardPath() {
        return Environment.getExternalStorageDirectory().getPath();
    }
    public static native void play(String inputurl, String outputurl);
    public static AudioTrack createAudioTrack() {
        int sampleRateInHz = 44100;
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        int channelConfig = android.media.AudioFormat.CHANNEL_OUT_STEREO;
        int bufferSizeInBytes = AudioTrack.getMinBufferSize(sampleRateInHz,
                channelConfig, audioFormat);
        AudioTrack audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,
                sampleRateInHz, channelConfig, audioFormat, bufferSizeInBytes,
                AudioTrack.MODE_STREAM);
        // audioTrack.write(audioData, offsetInBytes, sizeInBytes);
        return audioTrack;
    }
}
