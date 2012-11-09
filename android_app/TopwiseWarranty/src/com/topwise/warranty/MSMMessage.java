package com.topwise.warranty;


import android.content.Context;
import android.telephony.TelephonyManager;
//import android.telephony.gsm.SmsManager;
import android.telephony.SmsManager;

import com.topwise.warranty.utils.Log;

/**
 * 
 * @author zxsu
 * 
 * Send a msm message include IMEI to server.
 * 
 */
public class MSMMessage extends WMessage {

    //private Phone mPhone = null;
    private Context mContext;
    private String serverPhone;
    
    public MSMMessage(Context context) {
        super();
        
        mContext = context;
        serverPhone = context.getResources().getString(R.string.serverPhone);
        
    }
    
    public void buildMessage() {
        super.buildMessage();
        
        TelephonyManager telephonyManager = (TelephonyManager)mContext.
                getSystemService(Context.TELEPHONY_SERVICE);
        
        String imei = telephonyManager.getDeviceId();
        mMessage = imei;
    }
    
    //TODO: mobile network did not ready before sendMesssage
    public void sendMessage() {
        super.sendMessage();
        SmsManager.getDefault().sendTextMessage(
                serverPhone, null, mMessage, null, null);
        
        Log.d("in MSMMessage: " + mMessage);
    }
}
