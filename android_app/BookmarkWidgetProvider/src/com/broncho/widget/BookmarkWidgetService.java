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

import java.util.ArrayList;
import java.util.HashMap;

import android.app.PendingIntent;
import android.app.Service;
import android.appwidget.AppWidgetManager;
import android.content.ContentResolver;
import android.content.Intent;
import android.database.ContentObserver;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler; 

import android.provider.Browser;
import android.provider.Browser.BookmarkColumns;
import android.view.View;
import android.widget.RemoteViews;

import com.broncho.util.Log;

public class BookmarkWidgetService extends Service {

    /** the Screen ID want to display */
    public static final String CURRENT_SCREEN_ID = "currentScreenID";
    /** Force the bookmarks to be re-renderer. */
    public static final String UPDATE = "com.broncho.bookmarkwidget.UPDATE";

    /** Change the widget to the next bookmark. */
    private static final String NEXT = "com.broncho.bookmarkwidget.NEXT";

    /** Change the widget to the previous bookmark. */
    private static final String PREV = "com.broncho.bookmarkwidget.PREV";

    /** change the widget's modes **/
    private static final String CHANGMODE = "com.broncho.bookmarkwidget.CHANGMODE";
    
    /** View mode */
    public static final String VIEWMODE = "viewmode";
    public static final int GRIDVIEW = 0;
    public static final int LISTVIEW = 1;

    /** delete action */
    public static final String DELETE = "com.broncho.bookmarkwidget.delete";

    /** items per screen page **/
    public static final int ITEMNUMB = 4;
    private int mScreenNum;
    private int mCurrentScreen;
    private int mLastScreenItemNum;
    private int mAppWidgetId;
    private int mViewMode;

    private static final int[] TITLE_GRID = { R.id.title_tl, R.id.title_tr,
            R.id.title_bl, R.id.title_br };

    private static final int[] IMAGE_GRID = { R.id.image_tl, R.id.image_tr,
            R.id.image_bl, R.id.image_br };

    private static final int[] TITLE_LIST = { R.id.title_1, R.id.title_2,
            R.id.title_3, R.id.title_4 };

    private static final int[] FAVICON_LIST = { R.id.favicon_1, R.id.favicon_2,
            R.id.favicon_3, R.id.favicon_4 };

    private static final int[] URL_LIST = { R.id.url_1, R.id.url_2, R.id.url_3,
            R.id.url_4 };

    
    // TODU:XXX BOOKMARK ITEM CLASS maybe
    // Id -> information map storing db ids and their result.:::
    private final HashMap<Integer, BookMarkRenderResult> mDbId2BookMarkRenderResult = new HashMap<Integer, BookMarkRenderResult>();

    // List of db ids in order:::
    private final ArrayList<Integer> mDbIdList = new ArrayList<Integer>();

    // Map of AppWidget id to AppWidgetItem:::
    private HashMap<Integer, AppWidgetItem> mWidgetId2WidgetItem = new HashMap<Integer, AppWidgetItem>();

    // List of appids in order:::
    private final ArrayList<Integer> mWidgetIdList = new ArrayList<Integer>();

    // The current id used by the widget during an update.

    // ContentResolver
    private final ContentObserver mObserver = new ContentObserver(new Handler()) {
        public void onChange(boolean selfChange) {
            Log.d("reciver Observer Event!!!");
        }
    };

    @Override
    public void onCreate() {
        Log.d("In BookmarkWidgetService onCreate()!!");

        ContentResolver resolver = getContentResolver();
        resolver.registerContentObserver(Uri.withAppendedPath(
                Browser.BOOKMARKS_URI, BookmarkColumns.BOOKMARK + "==1"), true,
                mObserver);
    }

    @Override
    public void onDestroy() {
        Log.d("In BookmarkWidgetService onDestroy!!");
        ContentResolver resolver = getContentResolver();
        resolver.unregisterContentObserver(mObserver);
    }

    @Override
    public android.os.IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // TODU: bugs here, intent may be null or make a lot of jobs in
        // onstartCommand, do jobs in a thread
        String action = null;
        Bundle extras = null;
        if (intent != null) {
            action = intent.getAction();
            extras = intent.getExtras();
        }

        //Bundle extras = intent.getExtras();
        if (extras != null) {
            mAppWidgetId = extras.getInt(AppWidgetManager.EXTRA_APPWIDGET_ID,
                    AppWidgetManager.INVALID_APPWIDGET_ID);

            if (UPDATE.equals(action)) {
                mCurrentScreen = extras.getInt(CURRENT_SCREEN_ID, 0);
                mViewMode = extras.getInt(VIEWMODE, 0);

                if (!mWidgetIdList.contains(mAppWidgetId)) {
                    AppWidgetItem appWidgetItem = new AppWidgetItem(
                            mAppWidgetId, PREV, NEXT, CHANGMODE, mViewMode);
                    mWidgetId2WidgetItem.put(mAppWidgetId, appWidgetItem);
                    mWidgetIdList.add(mAppWidgetId);
                    Log.d("add the mAppWidgetId to List!!");
                }

                updateWidget();
                Log.d("recive UPDATE action in onStartCommand");
            } else if (DELETE.equals(action)) {
                if (mWidgetIdList.contains(mAppWidgetId)) {
                    mWidgetIdList.remove(mWidgetIdList.indexOf(mAppWidgetId));
                    mWidgetId2WidgetItem.remove(mAppWidgetId);
                }
            } else if (filteAction(action)) {
                mCurrentScreen = extras.getInt(CURRENT_SCREEN_ID, 0);
                mViewMode = extras.getInt(VIEWMODE, 0);
                BookmarkConfigActivity.saveCurrentScreenPref(getBaseContext(), mAppWidgetId, mCurrentScreen);
                BookmarkConfigActivity.saveModePref(getBaseContext(), mAppWidgetId, mViewMode);
                Log.d("Save the mViewMode is: " + mViewMode);
                updateWidget();
            }
        }

        return START_STICKY;
    }

    private boolean filteAction(String action) {
        int N = mWidgetIdList.size();
        for (int i = 0; i < N; i++) {
            AppWidgetItem item = mWidgetId2WidgetItem.get(mWidgetIdList.get(i));
            if (action.equals(item.nextAction)
                    || action.equals(item.prevAction) || action.equals(item.changeModeAction)) {
                return true;
            }
        }
        return false;
    }

    private int getNextScreen() {
        if (mCurrentScreen < mScreenNum - 1) {
            return mCurrentScreen + 1;
        } else {
            return mScreenNum - 1;
        }

    }

    private int getPrevScreen() {
        if (mCurrentScreen > 0) {
            return mCurrentScreen - 1;
        } else {
            return 0;
        }
    }

    private void updateWidget() {
        queryCursorAndRender();
        RemoteViews views = new RemoteViews(getPackageName(),
                R.layout.bookmarkwidget);
        RemoteViews item_views = null;

        AppWidgetItem item = mWidgetId2WidgetItem.get(mAppWidgetId);

        Intent prev = new Intent(item.prevAction, null, this,
                BookmarkWidgetService.class);
        prev.putExtra(CURRENT_SCREEN_ID, getPrevScreen());
        prev.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, mAppWidgetId);
        prev.putExtra(VIEWMODE, mViewMode);

        views.setOnClickPendingIntent(R.id.previous, PendingIntent.getService(
                this, 0, prev, PendingIntent.FLAG_UPDATE_CURRENT));

        Intent next = new Intent(item.nextAction, null, this,
                BookmarkWidgetService.class);
        next.putExtra(CURRENT_SCREEN_ID, getNextScreen());
        next.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, mAppWidgetId);
        next.putExtra(VIEWMODE, mViewMode);

        views.setOnClickPendingIntent(R.id.next, PendingIntent.getService(this,
                0, next, PendingIntent.FLAG_UPDATE_CURRENT));

        //add viewMode change func, what ever the "easter eggs"
        Intent modeChange = new Intent(item.changeModeAction, null, this,
                BookmarkWidgetService.class);
        modeChange.putExtra(CURRENT_SCREEN_ID, mCurrentScreen);
        modeChange.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, mAppWidgetId);
        modeChange.putExtra(VIEWMODE, getNextViewMode());
        
        views.setOnClickPendingIntent(R.id.title, PendingIntent.getService(this, 
                0, modeChange, PendingIntent.FLAG_UPDATE_CURRENT));
        // only when the bookmark have more the one screen, set the Next Button
        // invisible
        if (mCurrentScreen == mScreenNum - 1 && mScreenNum > 1) {
            views.setViewVisibility(R.id.next, View.INVISIBLE);
            // itemNum = mLastScreenItemNum;
        } else {
            views.setViewVisibility(R.id.next, View.VISIBLE);
        }

        if (mCurrentScreen == 0) {
            views.setViewVisibility(R.id.previous, View.INVISIBLE);
        } else {
            views.setViewVisibility(R.id.previous, View.VISIBLE);
        }
        // RemoteViews item_views = setBookmarkScreen(views, mCurrentScreen);
        if (mViewMode == LISTVIEW) {
            item_views = getBookmarkListView();
        } else {
            item_views = getBookmarkGridView();
        }

        views.removeAllViews(R.id.bookmark_container);
        views.addView(R.id.bookmark_container, item_views);

        AppWidgetManager.getInstance(this).updateAppWidget(mAppWidgetId, views);

    }

    private int getNextViewMode() {
        // TODO Auto-generated method stub
        if (mViewMode == LISTVIEW) {
            return GRIDVIEW;
        } else {
            return LISTVIEW;
        }
        
    }

    private RemoteViews getBookmarkListView() {
        //int begin = mCurrentScreen * ITEMNUMB;
        int itemNum = ITEMNUMB;

        RemoteViews itemViews = new RemoteViews(getPackageName(),
                R.layout.list_view);

        if (mCurrentScreen == mScreenNum - 1) {
            itemNum = mLastScreenItemNum;
        }

        for (int i = 0; i < itemNum; i++) {
            BookMarkRenderResult res = mDbId2BookMarkRenderResult.get(mDbIdList
                    .get(i));

            if (res == null) {
                break;
            }

            String displayTitle = res.mTitle;
            if (displayTitle == null) {
                displayTitle = res.mUrl;
            }
            itemViews.setTextViewText(TITLE_LIST[i], displayTitle);
            itemViews.setTextViewText(URL_LIST[i], res.mUrl);

            if (res.mFavicon != null) {
                itemViews.setImageViewBitmap(FAVICON_LIST[i], res.mFavicon);
            } else {
                itemViews.setImageViewResource(FAVICON_LIST[i],
                        R.drawable.app_web_browser_sm);
            }

            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setData(Uri.parse(res.mUrl));
            itemViews.setOnClickPendingIntent(URL_LIST[i], PendingIntent
                    .getActivity(this, 0, intent,
                            PendingIntent.FLAG_UPDATE_CURRENT));
            itemViews.setOnClickPendingIntent(TITLE_LIST[i], PendingIntent
                    .getActivity(this, 0, intent,
                            PendingIntent.FLAG_UPDATE_CURRENT));
        }

        if (itemNum != 4) {
            for (int i = itemNum; i < 4; i++) {
                itemViews.setViewVisibility(FAVICON_LIST[i], View.INVISIBLE);
            }
        }
        return itemViews;
    }

    private RemoteViews getBookmarkGridView() {
        //int begin = mCurrentScreen * ITEMNUMB;
        int itemNum = ITEMNUMB;

        RemoteViews itemView = new RemoteViews(getPackageName(),
                R.layout.grid_view);

        if (mCurrentScreen == mScreenNum - 1) {
            itemNum = mLastScreenItemNum;
        }

        for (int i = 0; i < itemNum; i++) {
            BookMarkRenderResult res = mDbId2BookMarkRenderResult.get(mDbIdList
                    .get(i));

            if (res == null) {
                break;
            }
            String displayTitle = res.mTitle;
            if (displayTitle == null) {
                displayTitle = res.mUrl;
            }
            itemView.setViewVisibility(TITLE_GRID[i], View.VISIBLE);
            itemView.setTextViewText(TITLE_GRID[i], displayTitle);

            if (res.mThumbnail != null) {
                itemView.setImageViewBitmap(IMAGE_GRID[i], res.mThumbnail);
            } else {
                itemView.setImageViewResource(IMAGE_GRID[i],
                        R.drawable.browser_thumbnail);
            }

            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setData(Uri.parse(res.mUrl));

            itemView.setOnClickPendingIntent(IMAGE_GRID[i], PendingIntent
                    .getActivity(this, 0, intent,
                            PendingIntent.FLAG_UPDATE_CURRENT));
        }

        if (itemNum != 4) {
            for (int i = itemNum; i < 4; i++) {
                itemView.setImageViewResource(IMAGE_GRID[i],
                        R.drawable.browser_thumbnail);
                itemView.setViewVisibility(TITLE_GRID[i], View.INVISIBLE);
            }
        }
        return itemView;
    }

    /**
     * private RemoteViews setBookmarkScreen(RemoteViews views, int
     * mCurrentScreen) {
     * 
     * int begin = mCurrentScreen * 4; int itemNum = 4;
     * 
     * if (mCurrentScreen == mScreenNum - 1) {
     * views.setViewVisibility(R.id.next, View.INVISIBLE); itemNum =
     * mLastScreenItemNum; } else { views.setViewVisibility(R.id.next,
     * View.VISIBLE); }
     * 
     * if (mCurrentScreen == 0) { views.setViewVisibility(R.id.previous,
     * View.INVISIBLE); } else { views.setViewVisibility(R.id.previous,
     * View.VISIBLE); }
     * 
     * for (int i = 0; i < itemNum; i++) { BookMarkRenderResult res =
     * mDbId2BookMarkRenderResult.get(mDbIdList.get(begin + i));
     * 
     * if (res == null) { break; } String displayTitle = res.mTitle; if
     * (displayTitle == null) { displayTitle = res.mUrl; }
     * views.setViewVisibility(TITLE_GRID[i], View.VISIBLE);
     * views.setTextViewText(TITLE_GRID[i], displayTitle);
     * 
     * if (res.mThumbnail != null) { views.setImageViewBitmap(IMAGE_GRID[i],
     * res.mThumbnail); } else { views.setImageViewResource(IMAGE_GRID[i],
     * R.drawable.browser_thumbnail); }
     * 
     * Intent intent = new Intent(Intent.ACTION_VIEW);
     * intent.setData(Uri.parse(res.mUrl));
     * 
     * views.setOnClickPendingIntent(IMAGE_GRID[i], PendingIntent.getActivity(
     * this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT));
     * 
     * }
     * 
     * if (itemNum != 4) { for (int i = itemNum; i < 4; i++) {
     * views.setImageViewResource(IMAGE_GRID[i], R.drawable.browser_thumbnail);
     * views.setViewVisibility(TITLE_GRID[i], View.INVISIBLE); } }
     * 
     * return views; }
     **/

    // Default WHERE clause is all bookmarks.
    private static final String QUERY_WHERE = BookmarkColumns.BOOKMARK
            + " == 1";
    private final String sortOrder = Browser.BookmarkColumns.VISITS + " DESC";

    private static final String THUMBNAIL = "thumbnail";
    private static final String[] PROJECTION = new String[] {
            BookmarkColumns._ID, BookmarkColumns.TITLE, BookmarkColumns.URL,
            THUMBNAIL, BookmarkColumns.FAVICON };

    // Class containing the rendering information for a specific bookmark item.
    private static class BookMarkRenderResult {
        final int mId;
        final String mTitle;
        final String mUrl;
        Bitmap mThumbnail;
        Bitmap mFavicon;

        BookMarkRenderResult(int id, String title, String url,
                Bitmap thumbnail, Bitmap favicon) {
            mId = id;
            mTitle = title;
            mUrl = url;
            mThumbnail = thumbnail;
            mFavicon = favicon;
        }
    }

    private static class AppWidgetItem {
        int appWidgetId;
        int viewMode;
        String prevAction;
        String nextAction;
        String changeModeAction;

        public AppWidgetItem(int id, String prev, String next, String changeMode, int mode) {
            appWidgetId = id;
            prevAction = prev + id;
            nextAction = next + id;
            changeModeAction = changeMode + id;
            viewMode = mode;
        }
    }

    /**
     * // TODU:XXX private void queryCursorAndRender() { // Clear the ordered
     * list of ids and the map of ids to bitmaps. // TODU:XXX mDbIdList.clear();
     * mDbId2BookMarkRenderResult.clear();
     * 
     * // Look up all the bookmarks Cursor c =
     * getContentResolver().query(Browser.BOOKMARKS_URI, PROJECTION,
     * QUERY_WHERE, null, sortOrder); if (c != null) { mScreenNum =
     * getScreenNum(c);
     * 
     * if ( mCurrentScreen > mScreenNum - 1 ) { mCurrentScreen = mScreenNum - 1;
     * }
     * 
     * mLastScreenItemNum = (c.getCount() % ITEMNUMB == 0) ? ITEMNUMB :
     * c.getCount() % ITEMNUMB;
     * 
     * if (c.moveToFirst()) {
     * 
     * do { int id = c.getInt(0); String title = c.getString(1); String url =
     * c.getString(2); Bitmap thumbnail = getBitMap(c.getBlob(3)); Bitmap
     * favicon = getBitMap(c.getBlob(4));
     * 
     * // Linear list of ids to obtain the previous and next. mDbIdList.add(id);
     * 
     * // Map the url to its db id for lookup when complete. mUrlsToIds.put(url,
     * id);
     * 
     * // Store the current information to at least display the // title.
     * BookMarkRenderResult res = new BookMarkRenderResult(id, title, url,
     * thumbnail, favicon);
     * 
     * mDbId2BookMarkRenderResult.put(id, res);
     * 
     * // Add the url to our list to render. // urls.add(url); } while
     * (c.moveToNext());
     * 
     * } c.close(); } }
     **/

    // TODU:XXX
    private void queryCursorAndRender() {
        // Clear the ordered list of ids and the map of ids to bitmaps.
        // TODU:XXX

        mDbIdList.clear();
        mDbId2BookMarkRenderResult.clear();

        // Look up all the bookmarks
        Cursor c = getContentResolver().query(Browser.BOOKMARKS_URI,
                PROJECTION, QUERY_WHERE, null, sortOrder);
        if (c != null) {
            mScreenNum = getScreenNum(c);

            if (mCurrentScreen > mScreenNum - 1) {
                mCurrentScreen = mScreenNum - 1;
            }

            mLastScreenItemNum = (c.getCount() % ITEMNUMB == 0) ? ITEMNUMB : c
                    .getCount()
                    % ITEMNUMB;

            c.moveToPosition(mCurrentScreen * ITEMNUMB);
            int i = 0;
            do {
                i++;
                int id = c.getInt(0);
                String title = c.getString(1);
                String url = c.getString(2);
                Bitmap thumbnail = getBitMap(c.getBlob(3));
                Bitmap favicon = getBitMap(c.getBlob(4));

                // Linear list of ids to obtain the previous and next.
                mDbIdList.add(id);

                // Store the current information to at least display the
                // title.
                BookMarkRenderResult res = new BookMarkRenderResult(id, title,
                        url, thumbnail, favicon);

                mDbId2BookMarkRenderResult.put(id, res);
            } while (c.moveToNext() && i < ITEMNUMB);

            c.close();
        }
    }

    private int getScreenNum(Cursor c) {
        int mNum = c.getCount() >> 2;
        if ((c.getCount() % ITEMNUMB) == 0) {
            return mNum;
        } else {
            return mNum + 1;
        }
    }

    private Bitmap getBitMap(byte[] data) {
        if (data == null) {
            return null;
        }

        return BitmapFactory.decodeByteArray(data, 0, data.length);
    }

}
