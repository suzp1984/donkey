package com.zhixin.zxlbs.park.util;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.util.Iterator;
import java.util.List;

import org.xmlpull.v1.XmlPullParser;

import android.util.Xml;

import com.zhixin.zxlbs.park.util.Log;

public class NavigationSettings {
    
    public final static String REARVIEW_MIRROR_CONFIG_PATH = "/data/etc/rearview_mirror.xml";
    
    private final static String PARK_ALARM_DISTANCE = "park_alarm_distance";
    private final static String PARK_ALARM_DESTINATION_NUMBER = "park_alarm_destination_number";
    private final static String PARK_ALARM_MESSAGE = "park_alarm_message";
    private final static String PARK_ALARM_INTERVAL = "park_alarm_interval";
    private final static String CRASH_ALARM_ACCURACY = "crash_alarm_accuracy";
    private final static String CRASH_ALARM_MESSAGE = "crash_alarm_message";
    private final static String CRASH_ALARM_DESTINATION_NUMBER = "crash_alarm_destination_number";
    
    private static String mAlarmDistance;
    private static String mAlarmDestinationNumber;
    private static String mAlarmMessage;
    private static String mAlarmInterval;
    private static String mCrashAccuracy;
    private static String mCrashMessage;
    private static String mCrashDestinationNumber;
    
    public NavigationSettings(String alarmDistance, String alarmDestinationNumber, 
            String alarmMessage, String alarmInterval, String crashAccuracy, 
            String crashMessage, String crashDestinationNumber) {
        mAlarmDistance = alarmDistance;
        mAlarmDestinationNumber = alarmDestinationNumber;
        mAlarmMessage = alarmMessage;
        mAlarmInterval = alarmInterval;
        mCrashAccuracy = crashAccuracy;
        mCrashMessage = crashMessage;
        mCrashDestinationNumber = crashDestinationNumber;
    }
    
    public float getAlarmDistance() {
        return Float.valueOf(mAlarmDistance);
    }
    
    public long getAlarmInterval() {
        // maybe >> 3 is better. form min -> milliseconds
        return Long.valueOf(mAlarmInterval) * 1000 * 60;
    }
    
    public String getAlarmPhoneNum() {
        return mAlarmDestinationNumber;
    }
    
    public String getAlarmMessage() {
        return mAlarmMessage;
    }

    public float getCrashAccuracy() {
        return Float.valueOf(mCrashAccuracy);
    }
    
    public String getCrashMessage() {
        return mCrashMessage;
    }
    
    public String getCrashDestinationNumber() {
        return mCrashDestinationNumber;
    }
    
    public static NavigationSettings getNavigationSettingsFromConfig() {
        /**
        final File configFile = new File(REARVIEW_MIRROR_CONFIG_PATH);
        try {
            FileReader rearviewmirrorReader = new FileReader(configFile);
            try {
                XmlPullParser parser = Xml.newPullParser();
                String alarmDistance = null;
                String alarmDestinatonNumber = null;
                String alarmMessage = null;
                String alarmInterval = null;
                
                parser.setInput(rearviewmirrorReader);
                
            }
        }**/
        SaxXmlParser parser = new SaxXmlParser(REARVIEW_MIRROR_CONFIG_PATH);
        List<Item> items = parser.parse();
        Iterator<Item> iterator = items.iterator();
        
        while(iterator.hasNext()) {
            Item item = iterator.next();
            if(PARK_ALARM_DISTANCE.equals(item.getId())) {
                mAlarmDistance = item.getValue();
            } else if (PARK_ALARM_DESTINATION_NUMBER.equals(item.getId())) {
                mAlarmDestinationNumber = item.getValue();
            } else if (PARK_ALARM_MESSAGE.equals(item.getId())) {
                mAlarmMessage = item.getValue();
            } else if (PARK_ALARM_INTERVAL.equals(item.getId())) {
                mAlarmInterval = item.getValue();
            } else if (CRASH_ALARM_ACCURACY.equals(item.getId())) {
                mCrashAccuracy = item.getValue();
            } else if (CRASH_ALARM_MESSAGE.equals(item.getId())) {
                mCrashMessage = item.getValue();
            } else if (CRASH_ALARM_DESTINATION_NUMBER.equals(item.getId())) {
                mCrashDestinationNumber = item.getValue();
            }
        }
        
        return new NavigationSettings(mAlarmDistance, mAlarmDestinationNumber, 
                mAlarmMessage, mAlarmInterval, mCrashAccuracy, mCrashMessage,
                mCrashDestinationNumber);
        
       
    } 
}
