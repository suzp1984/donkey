package com.zhixin.zxlbs.park;

import java.util.Locale;

import android.app.Service;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.speech.tts.TextToSpeech;
import android.speech.tts.TextToSpeech.OnInitListener;
import com.zhixin.zxlbs.park.util.Log;

public class TtsService extends Service implements OnInitListener {

    private static final String TAG = "TTSSERVICE";
    public  static final String TTSACTION = "com.zhixin.zxlbs.park.tts.text";
    public static final String TTSTEXT = "text";
    
    private TextToSpeech mTts;
    private String mText;
    
    

    @Override
    public void onCreate() {
        Log.d("TtsService: in oncreate");
        mTts = new TextToSpeech(this, this);
    }
    
    @Override
    public void onStart(Intent intent, int startId) {
        String action = null;
        if (intent != null) {
            action = intent.getAction();
        }
        Log.d("TtsService: in onStart");
        
        if(TTSACTION.equals(action)) {
            Bundle bundle = intent.getExtras();
            mText = bundle.getString(TTSTEXT);
            Log.d("TtsService: get text is " + mText);
            if(mText != null) {
                mTts.speak(mText, TextToSpeech.QUEUE_FLUSH, null);
            }
        }
    }
    
    @Override
    public IBinder onBind(Intent arg0) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public void onDestroy() {
        if(mTts != null) {
            mTts.stop();
            mTts.shutdown();
        }
        
        super.onDestroy();
    }
    public void onInit(int status) {
        if (status == TextToSpeech.SUCCESS) {
            int result = mTts.setLanguage(Locale.getDefault());
            
            if(result == TextToSpeech.LANG_MISSING_DATA ||
                    result == TextToSpeech.LANG_NOT_SUPPORTED) {
                Log.e("TtsService: not support local language");
            }
        } else {
            Log.e("TtsService: error in init TTS");
        }
        
        if(mText != null) {
            mTts.speak(mText, TextToSpeech.QUEUE_FLUSH, null);
        }

    }

}
