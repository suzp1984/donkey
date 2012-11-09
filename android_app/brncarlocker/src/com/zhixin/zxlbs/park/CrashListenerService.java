package com.zhixin.zxlbs.park;

import com.zhixin.zxlbs.park.util.Log;
import com.zhixin.zxlbs.park.util.NavigationSettings;

import android.app.Service;
import android.content.Intent;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;

import android.os.IBinder;
import android.telephony.gsm.SmsManager;

public class CrashListenerService extends Service implements
        SensorEventListener {

    private SensorManager mgr;
    private NavigationSettings settings = NavigationSettings
            .getNavigationSettingsFromConfig();

    public void onCreate() {
        super.onCreate();
        Log.d("start Crash Services now!!");
        mgr = (SensorManager) getSystemService(SENSOR_SERVICE);

        Sensor sensor = mgr.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        mgr.registerListener(this, sensor, SensorManager.SENSOR_DELAY_NORMAL);
    }

    @Override
    public void onStart(Intent intent, int startId) {
        Log.d("in Crash Services onStart!");
    }

    @Override
    public void onDestroy() {
        Log.d("Stop Crash Services now!!");
        mgr.unregisterListener(this);
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {

        return null;
    }

    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        // not needed

    }

    public void onSensorChanged(SensorEvent event) {
        for (int i = 1; i < event.values.length; i++) {
            Log.d("get sensor values[" + i + "]: " + event.values[i]);
            if (Math.abs(event.values[i]) > getCrashAccuracy()) {
                crashWarning();
                Log.d("send Warning values[" + i + "]: " + event.values[i]);
            }
        }

    }

    private float getCrashAccuracy() {
        return settings.getCrashAccuracy();

    }

    private String getCrashMessage() {
        return settings.getCrashMessage();
    }

    private String getCrashDestinationNum() {

        return settings.getCrashDestinationNumber();
    }

    private void crashWarning() {

        SmsManager.getDefault().sendTextMessage(getCrashDestinationNum(), null,
                getCrashMessage(), null, null);

        // tts warning
        Intent ttsIntent = new Intent(getApplicationContext(), TtsService.class);
        ttsIntent.setAction(TtsService.TTSACTION);
        ttsIntent.putExtra(TtsService.TTSTEXT, getResources()
                .getString(R.string.car_crash));
        startService(ttsIntent);
        Log.d("send Crash Warning!!");

    }

}
