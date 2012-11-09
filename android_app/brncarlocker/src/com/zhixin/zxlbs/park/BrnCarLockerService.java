package com.zhixin.zxlbs.park;

import java.util.Timer;
import java.util.TimerTask;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;

import android.content.Context;
import android.content.Intent;

import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;

import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder; 
import android.os.PowerManager;
import android.telephony.gsm.SmsManager;

import com.zhixin.zxlbs.park.util.Log;

import com.zhixin.zxlbs.park.util.NavigationSettings;

public class BrnCarLockerService extends Service implements LocationListener {

    private LocationManager locationmanager;
    private Location lockedLocation = null;
    private boolean mLockCar = false;

    public static final String PARK = "android.intent.action.PARK_BUTTON";
    public static final String CONFIG_CHANGE = "com.zhixin.CONFIG_CHANGED";
    public static final String LOCK_NAME_STATIC = "com.zhixin.LOCK_STATIC";
    public static final int ACCURACY_DISTANCE = 300;

    private static PowerManager.WakeLock wakeLocker = null;
    
    private NavigationSettings settings = NavigationSettings
            .getNavigationSettingsFromConfig();
    /*
     * A1 test notification
     */
    private NotificationManager notificationManager;

    private boolean onCreateWasCalled = false;

    private final Handler handler = new Handler();

    private final Timer timer = new Timer();

    /**
     * Task invoked by a timer periodically to make sure the loction listener is
     * still registered.
     */
    private final TimerTask checkLocationListener = new TimerTask() {

        @Override
        public void run() {
            if (!onCreateWasCalled) {
                Log.e("BrnCarLockerService is running, "
                        + "But onCreate not called");
            }

            if (isCarLocking()) {
                Log.d("isCarLocking is true!!");
                
                // get last gps location if locakedLocation is null
                if (lockedLocation == null) {
                    lockedLocation = locationmanager.getLastKnownLocation("gps");
                }
                
                handler.post(new Runnable() {

                    public void run() {
                        Log.d("Re-registering Location "
                                + "listener in BrnCarLockerService!");
                        unregisterLocationListener();
                        registerLocationListener();
                    }
                });
            }

        }
    };

    public void onCreate() {

        Log.d("BrnCarLockerService oncreate");
        super.onCreate();

        // get wakelock here
        PowerManager mgr = (PowerManager) getSystemService(Context.POWER_SERVICE);
        wakeLocker =mgr.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, LOCK_NAME_STATIC);
        wakeLocker.acquire();
        
        notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);

        onCreateWasCalled = true;
        locationmanager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);

        //startService(new Intent(getApplicationContext(), TtsService.class));
        Log.d("LocationManager is going");
    }

    private void registerLocationListener(long time, float distance) {
        if (locationmanager == null) {
            Log.e("brnCarlockerservice do not have any location manager!");
            return;
        }
        
        locationmanager.requestLocationUpdates("gps", time, distance, this);
    }

    @Override
    public void onStart(Intent intent, int startId) {
        String action = null;
        if (intent != null) {
            action = intent.getAction();
        }
        
        Intent ttsIntent = new Intent(getApplicationContext(), TtsService.class);
        ttsIntent.setAction(TtsService.TTSACTION);
        if (PARK.equals(action)) {
            if (!isCarLocking()) {
                Log.d("lock the car !!!");
               
                ttsIntent.putExtra(TtsService.TTSTEXT, getResources()
                        .getString(R.string.lockcar));
                startService(ttsIntent);
                lockCar();
            } else {
                Log.d("unlock the car !!!");
                
                ttsIntent.putExtra(TtsService.TTSTEXT, getResources()
                        .getString(R.string.unlockcar));
                startService(ttsIntent);
                unlockCar();

            }
        } else if (CONFIG_CHANGE.equals(action)) {
            Log.d("get the xml config change signal!!");
            settings = NavigationSettings.getNavigationSettingsFromConfig();
        }
        return;
    }

    @Override
    public void onDestroy() {
        Log.d("In BrnCarLockerService onDestroy!!");
        // unlockCar();
        lockedLocation = null;
        mLockCar = false;
        unregisterLocationListener();
        wakeLocker.release();
        super.onDestroy();

    }

    private void registerLocationListener() {
        //regist location listener, get update as soon as possible
        registerLocationListener(0, 0);
        Log.d("register another LocationListener to get warnning");
    }

    // must be meters.
    private float getMinDistance() {
        return settings.getAlarmDistance();
    }

    // must be milliseconds, larger than 60000ms
    private long getMinTime() {
        return settings.getAlarmInterval();
    }

    private void unregisterLocationListener() {
        if (locationmanager == null) {
            Log.e("BrnCarLockerService do not have any location manager.");
            return;
        }

        locationmanager.removeUpdates(this);
        Log.d("Location listener now unregistered.");
    }

    @Override
    public IBinder onBind(Intent intent) {

        Log.d("brncarlockerservice.onbind");

        return null;
    }

    public void onLocationChanged(Location location) {

        if (location.getAccuracy() > ACCURACY_DISTANCE) {
            Log.d("Bad accuracy");
            return;
        }

        if (lockedLocation != null) {

            if (location.distanceTo(lockedLocation) > getMinDistance()) {
                warning(location);
            } else {
                Log.d("The distance to loackedLocation is"
                        + location.distanceTo(lockedLocation));
            }
        } else {
            lockedLocation = location;
            // assert the lockedlocationPolicy here;
            showNotification();

            Log.d("----------get the lockedLocation and change the listener policy----");
        }
        
        unregisterLocationListener();
    }

    private float getMaxDistance() {
        return 10000;
    }

    private void warning(Location location) {

        //SmsManager.getDefault().sendTextMessage(settings.getAlarmPhoneNum(),
         //       null, settings.getAlarmMessage(), null, null);

        Log.d("Warnning car is moving!!!!! "
                + location.distanceTo(lockedLocation));
        /**
        Log.d("the warning msg is " + settings.getAlarmMessage());
        Log.d("the phone number is " + settings.getAlarmPhoneNum());
        Log.d("the warning Interval is " + settings.getAlarmInterval());
        Log.d("the warning distance is " + settings.getAlarmDistance());
         **/
        showNotification();
    }

    private void showNotification() {
        if (isCarLocking()) {
            Notification notification = new Notification(R.drawable.arrow_320,
                    null, System.currentTimeMillis());

            PendingIntent contentIntent = PendingIntent.getActivity(this, 0,
                    new Intent(Intent.ACTION_VIEW), 0);
            notification.setLatestEventInfo(this, getString(R.string.app_name),
                    getString(R.string.app_name), contentIntent);

            notification.vibrate = new long[] { 100, 250, 100, 500 };

            notificationManager.notify(R.string.warning_message, notification);
        }
    }

    public void onProviderDisabled(String provider) {
        

    }

    public void onProviderEnabled(String provider) {
        

    }

    public void onStatusChanged(String provider, int status, Bundle extras) {
        

    }

    private void unlockCar() {

        unregisterLocationListener();
        timer.cancel();
        lockedLocation = null;
        mLockCar = false;
        stopSelf();
    }

    private void lockCar() {

        if (mLockCar == true) {
            Log.d("just return!!");
            return;
        }

        mLockCar = true;
        Log.d("set mLockcar to true!!");

        // regist logcation listener
        registerLocationListener();
        /**
         * After 5 * timeInterval min, check every timeInterval that location
         * listener still is registered
         */
        timer.schedule(checkLocationListener, getTimerDelay(),
                getTimerUpdatePeriod());

    }

    private long getTimerUpdatePeriod() {
        return settings.getAlarmInterval() < 170000 ? 170000 : settings
                .getAlarmInterval();
    }

    private long getTimerDelay() {
        return settings.getAlarmInterval() < 170000 ? 170000 * 5 : settings
                .getAlarmInterval() * 5;
    }

    private boolean isCarLocking() {

        return mLockCar;
    }

}
