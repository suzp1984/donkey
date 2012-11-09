package com.zhixin.zxlbs.gpsagent;

import com.zhixin.zxlbs.utils.Log;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class OnBootReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d("receiver the on BOOT_COMPILTE broadcast!!");
        
        context.startService(new Intent(context, GPSAgentService.class));

    }

}
