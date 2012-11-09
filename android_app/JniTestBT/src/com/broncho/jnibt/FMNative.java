package com.broncho.jnibt;

import com.broncho.jnibt.utils.Log;


public class FMNative {

	private int mNativeContext;
	
	private void get_channel_freq_cb(float param) {
		Log.e("get channel freq cb " + param);
	}
	
	private void get_current_rssi_cb(float param) {
		Log.e("get current rssi cb " + param);
	}
	
	private static native void class_init();
	private native void native_init();

	private native void send_cmd(String cmd, String cmdparam);
}
