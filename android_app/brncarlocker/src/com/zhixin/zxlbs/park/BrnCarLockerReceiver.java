package com.zhixin.zxlbs.park;

import com.zhixin.zxlbs.park.util.Log;


import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.KeyEvent;

public class BrnCarLockerReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {

        Log.d("BrnCarLockerRecevier Receive Boradcast!!");
        
        Intent parkService = new Intent(context, BrnCarLockerService.class);
        String parkAction = intent.getAction();

        if ("android.intent.action.PARK_BUTTON".equals(parkAction)) {
            // may be the long-press need not to listen
            Log.d("get PARK_BUTTON event");
            Bundle bundle = intent.getExtras();
            KeyEvent keyEvent = bundle.getParcelable(Intent.EXTRA_KEY_EVENT);
            if (KeyEvent.ACTION_UP == keyEvent.getAction() && KeyEvent.KEYCODE_BACK == keyEvent.getKeyCode()) {
                parkService.setAction("android.intent.action.PARK_BUTTON");
                Log.d("start Service");
                context.startService(parkService);
            }
        } else if (BrnCarLockerService.CONFIG_CHANGE.equals(parkAction)) {
            parkService.setAction(parkAction);
            context.startService(parkService);
        } 

        return;
    }

}
