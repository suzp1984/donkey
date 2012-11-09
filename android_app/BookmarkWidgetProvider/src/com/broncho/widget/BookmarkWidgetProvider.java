/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.broncho.widget;

import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.Context;
import android.content.Intent;
import com.broncho.util.Log;

enum BookmarkViewMode {
    GRID, LIST
};

/**
 * Widget that shows a preview of the user's bookmarks.
 */
public class BookmarkWidgetProvider extends AppWidgetProvider {

    // static final String TAG = "BookmarkWidgetProvider";

    @Override
    public void onUpdate(Context context, AppWidgetManager mngr, int[] ids) {
        int N = ids.length;
        for (int i = 0; i < N; i++) {
            defaultUpdateWidget(context, ids[i]);
            /**
            Intent intent = new Intent(BookmarkWidgetService.UPDATE,
                    null, context, BookmarkWidgetService.class);
            intent.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, ids[i]);
            intent.putExtra(BookmarkWidgetService.CURRENT_SCREEN_ID, 0);
            intent.putExtra(BookmarkWidgetService.VIEWMODE, 
                    BookmarkWidgetService.GRIDVIEW);
            context.startService(intent);
            **/
            Log.d("in appwidgetProvider onUpdate() startservice " +
            		"with appwidgetId" + ids[i]);
        }
    }

    private void defaultUpdateWidget(Context context, int appWidgetId) {
        int viewMode = BookmarkConfigActivity.loadModePref(context, appWidgetId);
        int currentScreen = BookmarkConfigActivity.loadCurrentScreenPref(context, appWidgetId);
        setWidgetState(context, appWidgetId, viewMode, currentScreen);        
    }

    @Override
    public void onEnabled(Context context) {
        context.startService(new Intent(context, BookmarkWidgetService.class));
        Log.d("in appwidgetProvider onEnabled()");
    }

    @Override
    public void onDisabled(Context context) {
        context.stopService(new Intent(context, BookmarkWidgetService.class));

        Log.d("in appwidgetProvider onDisabled()");
    }
    
    @Override
    public void onDeleted(Context context, int[] appWidgetIds) {
        int N = appWidgetIds.length;
        for (int i = 0; i < N; i++) {
            Intent intent = new Intent(BookmarkWidgetService.DELETE, 
                    null, context, BookmarkWidgetService.class);
            intent.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, appWidgetIds[i]);
            
            context.startService(intent);
            BookmarkConfigActivity.deleteWidgetIdPref(context, appWidgetIds[i]);
        }
    }

    public static void setWidgetState(Context context, int ids, int viewmode, int currentScreen) {
        
        Intent intent = new Intent(BookmarkWidgetService.UPDATE,
                null, context, BookmarkWidgetService.class);
        intent.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, ids);
        intent.putExtra(BookmarkWidgetService.CURRENT_SCREEN_ID, currentScreen);
        intent.putExtra(BookmarkWidgetService.VIEWMODE, 
                viewmode);
        context.startService(intent);
    }
}
