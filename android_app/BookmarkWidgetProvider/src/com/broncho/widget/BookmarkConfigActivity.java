package com.broncho.widget;

import android.app.Activity;
import android.appwidget.AppWidgetManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

public class BookmarkConfigActivity extends Activity {

    private final int[] IMAGE = {
            R.drawable.grid_mode,
            R.drawable.list_mode
    };
    
    private static final String PREFS_NAME = "com.broncho.widget.bookmark";
    private static final String PREF_PREFERENCE_MODE_KEY = "prefix_mode";
    private static final String PREF_PREFERENCE_SCREEN_KEY = "prefix_screen";
    
    private int mModeId = 0;
    
    private int mAppWidgetId = AppWidgetManager.INVALID_APPWIDGET_ID;

    View.OnClickListener mOnClickListener = new View.OnClickListener() {

		public void onClick(View v) {
			
			final Context context = BookmarkConfigActivity.this;
			  
			int modeNum = IMAGE.length;
			
			TextView next = (TextView)findViewById(R.id.widget_chooser_next);
			TextView prev = (TextView)findViewById(R.id.widget_chooser_previous);
			ImageView image = (ImageView)findViewById(R.id.image_chooser);


			if(v.equals(findViewById(R.id.widget_chooser_next))) {

				if(mModeId == modeNum - 1) {               
					next.setTextColor(getResources().getColor(R.color.unable));
					prev.setTextColor(getResources().getColor(R.color.enable));
				} else {
					mModeId++;
					prev.setTextColor(getResources().getColor(R.color.enable));
					
					next.setTextColor(getResources().getColor(R.color.enable));
					image.setImageResource(IMAGE[mModeId]);
					if (mModeId == modeNum - 1) {
						next.setTextColor(getResources().getColor(R.color.unable));

					}
				}


			} else if(v.equals(findViewById(R.id.widget_chooser_previous))) {

				if(mModeId == 0) {
					prev.setTextColor(getResources().getColor(R.color.unable));
					next.setTextColor(getResources().getColor(R.color.enable));
				} else {
					mModeId--;
					next.setTextColor(getResources().getColor(R.color.enable));
					prev.setTextColor(getResources().getColor(R.color.enable));
					image.setImageResource(IMAGE[mModeId]);
					if(mModeId == 0) {
						prev.setTextColor(getResources().getColor(R.color.unable));
					}
				}
			} else if (v.equals(findViewById(R.id.widget_chooser_select))) {
			    saveModePref(context, mAppWidgetId, mModeId);
			    saveCurrentScreenPref(context, mAppWidgetId, 0);
			    BookmarkWidgetProvider.setWidgetState(context, mAppWidgetId,
			              mModeId, 0);
			 // pass back the original appwidgetId
	            Intent resultValue = new Intent();
	            resultValue.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, mAppWidgetId);
	            setResult(RESULT_OK, resultValue);
	            finish();
			}
		}

        
	};

	static void saveModePref(Context context, int appWidgetId,
            int id) {
        SharedPreferences.Editor prefs = context.getSharedPreferences
        (PREFS_NAME, 0).edit();
        
        prefs.putInt(PREF_PREFERENCE_MODE_KEY + appWidgetId, id);        
        prefs.commit();     
    }
	
	static void saveCurrentScreenPref(Context context, int appWidgetId, 
	        int currenScreen) {
	    SharedPreferences.Editor prefs = context.getSharedPreferences(
	            PREFS_NAME, 0).edit();
	    
	    prefs.putInt(PREF_PREFERENCE_SCREEN_KEY + appWidgetId, currenScreen);
	    prefs.commit();
	}
	
	static int loadModePref(Context context, int appwidgetId) {
	    SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, 0);
	    int viewMode = prefs.getInt(PREF_PREFERENCE_MODE_KEY + appwidgetId, 0);
	    
	    return viewMode;
	}
	
	static int loadCurrentScreenPref(Context context, int appwidgetId) {
	    SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, 0);
	    int currentScreen = prefs.getInt(PREF_PREFERENCE_SCREEN_KEY + appwidgetId, 0);
	    
	    return currentScreen;
	}
	
	static void deleteWidgetIdPref(Context context, int appWidgetId) {
	    SharedPreferences.Editor prefs = context.getSharedPreferences(
	            PREFS_NAME, 0).edit();
	    
	    prefs.remove(PREF_PREFERENCE_MODE_KEY + appWidgetId);
	    prefs.remove(PREF_PREFERENCE_SCREEN_KEY + appWidgetId);
	    prefs.commit();
	}
	
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        setContentView(R.layout.main);

        findViewById(R.id.widget_chooser_next).setOnClickListener(mOnClickListener);
        findViewById(R.id.widget_chooser_previous).setOnClickListener(mOnClickListener); 
        findViewById(R.id.widget_chooser_select).setOnClickListener(mOnClickListener);
        // Find the widget id from the intent
        Intent intent = getIntent();
        Bundle extras = intent.getExtras();
        if (extras != null) {
            mAppWidgetId = extras.getInt(AppWidgetManager.EXTRA_APPWIDGET_ID,
                    AppWidgetManager.INVALID_APPWIDGET_ID);
        }

        if (mAppWidgetId == AppWidgetManager.INVALID_APPWIDGET_ID) {
            finish();
        }
    }

}
