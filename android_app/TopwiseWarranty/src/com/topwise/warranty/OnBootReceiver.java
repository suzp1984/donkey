package com.topwise.warranty;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.SystemClock;
import android.preference.Preference;
//import android.preference.

import com.topwise.warranty.utils.Log;

public class OnBootReceiver extends BroadcastReceiver {

    SharedPreferences mPreference;

    @Override
    public void onReceive(Context context, Intent intent) {
        // TODO Auto-generated method stub
        String action = intent.getAction();

        Log.d("OnBootReceiver " + action);

        mPreference = context.getSharedPreferences(
                WarrantyService.SETTING_STORE, Context.MODE_PRIVATE);

        if (!mPreference.getBoolean(WarrantyService.MESSAGE_SENDED, false)) {
            if (action.equals(Intent.ACTION_BOOT_COMPLETED)) {
                context.startService(new Intent(context, WarrantyService.class));
            } else if (action.equals(Intent.ACTION_SHUTDOWN)) {
                /*
                long milliSeconds = mPreference.getLong(WarrantyService.UPTIME, 0);
                milliSeconds += SystemClock.uptimeMillis();
                
                SharedPreferences.Editor editor = mPreference.edit();
                editor.putLong(WarrantyService.UPTIME, milliSeconds);
                editor.commit(); */
            }
        }

    }

}
