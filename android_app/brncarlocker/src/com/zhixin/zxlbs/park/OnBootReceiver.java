package com.zhixin.zxlbs.park;

import com.zhixin.zxlbs.park.util.Log;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class OnBootReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d("recevice the on BOOT_COMPLITE broadcast!");
        context.startService(new Intent(context, CrashListenerService.class));
    }

}
