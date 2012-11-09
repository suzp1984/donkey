#!/system/bin/sh

/system/bin/logcat >> /sdcard/main.log &
/system/bin/logcat -b radio >> /sdcard/radio.log &
/system/bin/logcat -b events >> /sdcard/events.log &
