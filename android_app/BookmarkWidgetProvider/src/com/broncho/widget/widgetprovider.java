package com.broncho.widget;

import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.Context;
import android.content.Intent;

public class widgetprovider extends AppWidgetProvider {
    
    @Override
    public void onUpdate(Context context, AppWidgetManager mngr, int[] ids) {
        context.startService(new Intent(widgetService.UPDATE, null,
                    context, widgetService.class));
    }

    @Override
    public void onEnabled(Context context) {
		context.startService(new Intent(context, widgetService.class));
    }
    
    @Override
    public void onDisabled(Context context) {
		context.stopService(new Intent(context, widgetService.class));
    }
}
