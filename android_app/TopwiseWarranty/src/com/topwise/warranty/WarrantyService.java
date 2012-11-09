package com.topwise.warranty;

import java.util.Timer;
import java.util.TimerTask;

import android.app.Service;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.SystemClock;

import com.topwise.warranty.utils.Log;

public class WarrantyService extends Service {

    public static final String SETTING_STORE = "SettingStore";
    public static final String MESSAGE_SENDED = "message_sended"; 
    public static final String UPTIME = "uptime";
    
    private static final int TYPE_MSM_MESSAGE = 0;
    private static final int TYPE_NET_MESSAGE = 1;
    private static final int MAX_MESSAGE_TYPE = TYPE_NET_MESSAGE;
    
    private static final int MESSAGE_CHECK_UPTIME = 1;
    private static long TASK_CHECK_PERIOD;
    
    private WMessage mMessages[];
    private  long defaultUptime;
    private long lastUptime;
    
    private SharedPreferences mPreferences;
    private Handler mHandler;
    private Timer mTimer;
    private MessageTimerTask mTimerTask;
    
    class MessageTimerTask extends TimerTask {

        @Override
        public void run() {
            // TODO Auto-generated method stub
            Log.d("***** TimerTask run ******");
            Message msg = mHandler.obtainMessage(MESSAGE_CHECK_UPTIME);
            msg.sendToTarget();
        }
        
    }
    
    @Override
    public void onCreate() {
        super.onCreate();
        
        mPreferences = getApplication()
                .getSharedPreferences(SETTING_STORE, MODE_PRIVATE);
        
        mMessages = new WMessage[MAX_MESSAGE_TYPE + 1];
        
        int[] messageTypes = getApplication().getResources()
                .getIntArray(R.array.messageTypes);
        
        defaultUptime = Long.parseLong(getApplication().
                getResources().getString(R.string.defaultUptime)); 
        TASK_CHECK_PERIOD = Long.parseLong(getApplication()
                .getResources().getString(R.string.taskFreqence));
        
        lastUptime = mPreferences.getLong(UPTIME, 0);
        
        for (int messageType : messageTypes) {
            switch (messageType) {
            case TYPE_MSM_MESSAGE:
                MSMMessage msmMessage = new MSMMessage(getApplicationContext());
                mMessages[TYPE_MSM_MESSAGE] = msmMessage;
                break;

            case TYPE_NET_MESSAGE:
                NetMessage netMessage = new NetMessage();
                mMessages[TYPE_NET_MESSAGE] = netMessage;
                break;
            default:
                break;
            }
        }
        
        mHandler = new Handler() {
            @Override
            public void handleMessage(Message message) {
                switch (message.what) {
                case MESSAGE_CHECK_UPTIME:
                    checkUpTime();                   
                    break;

                default:
                    break;
                }
            }
        };
        
        mTimer = new Timer();
        Log.d("WarrantyService oncreate");
    }
    
    private void checkUpTime() {
        Log.d("************ check uptime *******");
        
        long upTime = SystemClock.uptimeMillis();
        
        SharedPreferences.Editor editor = mPreferences.edit();
        editor.putLong(UPTIME, lastUptime + upTime); 
        editor.commit();
        
        if (defaultUptime > lastUptime + upTime) {
            if (mTimer != null) {
                if (mTimerTask != null) {
                    mTimerTask.cancel();
                }
                
                mTimerTask = new MessageTimerTask();
                mTimer.schedule(mTimerTask, TASK_CHECK_PERIOD);
            }
        } else {
            sendMessage();
        }
    }
    
    private boolean isMessageSended() {
        return mPreferences.getBoolean(MESSAGE_SENDED, false);
    }
    
    private void sendMessage() {
        for (WMessage message : mMessages) {
            if (message != null) {
                message.buildMessage();
                message.sendMessage();
            }
        }
        
        SharedPreferences.Editor editor = mPreferences.edit();
        editor.putBoolean(MESSAGE_SENDED, true);
        editor.commit();
    }
    
    private void handleCommand(Intent intent) {
        Log.d("handleCommand");
        
        if (isMessageSended()) {
            onDestroy();
            return;
        }
        
        checkUpTime();
        //sendMessage();
    }
    
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d("onstartCommand");
        handleCommand(intent);
        
        return START_NOT_STICKY;
        
    }
    
    @Override
    public IBinder onBind(Intent intent) {
        Log.d("onBind");
        
        return null;
    }
    
    @Override
    public void onDestroy() {
        Log.d("WarrantyService onDestroy");
        
        super.onDestroy();        
    }

}
