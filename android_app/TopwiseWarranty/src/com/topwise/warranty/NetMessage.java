package com.topwise.warranty;

import com.topwise.warranty.utils.Log;

public class NetMessage extends WMessage {

    public NetMessage() {
        super();
    }
    
    public void buildMessage() {
        super.buildMessage();
        
        mMessage = "build NetMessage";
    }
    
    public void sendMessage() {
        super.sendMessage();
        
        Log.d("In NetMessage: " + mMessage);
    }
}
