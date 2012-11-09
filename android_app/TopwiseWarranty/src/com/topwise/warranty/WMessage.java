package com.topwise.warranty;

import com.topwise.warranty.utils.Log;

public abstract class WMessage {

    protected String mMessage;
    
    public WMessage() {
    
    }
    
    public void buildMessage() {
        mMessage = "buildMessage";
    }
    
    public void sendMessage() {
        Log.d("in message: " + mMessage);
    }
}
