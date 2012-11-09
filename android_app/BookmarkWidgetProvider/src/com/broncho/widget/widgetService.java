package com.broncho.widget;

import android.app.Service;
import android.appwidget.AppWidgetManager;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.IBinder;
import android.provider.Browser;
import android.widget.RemoteViews;

import com.broncho.util.Log;

public class widgetService extends Service {


    private static final String TAG = "BookmarkWidgetService";

    /** Force the bookmarks to be re-renderer. */
    public static final String UPDATE = "com.android.browser.widget.UPDATE";

    /** Change the widget to the next bookmark. */
    private static final String NEXT = "com.android.browser.widget.NEXT";

    /** Change the widget to the previous bookmark. */
    private static final String PREV = "com.android.browser.widget.PREV";

    /** Id of the current item displayed in the widget. */
    private static final String EXTRA_ID =
            "com.android.browser.widget.extra.ID";

    private final String sortOrder = Browser.BookmarkColumns.VISITS + " DESC";

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        RemoteViews updateViews = buildRemoteViews();
        Log.d("get the updateViews.");

        ComponentName bookmarkWidget = new ComponentName(this,
                widgetprovider.class);
        AppWidgetManager manager = AppWidgetManager.getInstance(this);
        manager.updateAppWidget(bookmarkWidget, updateViews);
        Log.d("in onStartCommand, update bookmarkWidget");
        return 0;
    };

    private RemoteViews buildRemoteViews() {
        ContentResolver contentResolver = getContentResolver();
        Cursor mcursor = contentResolver.query(Browser.BOOKMARKS_URI,
                Browser.HISTORY_PROJECTION, Browser.BookmarkColumns.BOOKMARK
                        + " = 1", null, sortOrder);
        // TODU: need register a cotent observer
        int mCount = mcursor.getCount();
        int itemLines = mCount >> 1;
        RemoteViews mainViews = new RemoteViews(getPackageName(),
                R.layout.widget_bookmark);
        // TODU add nested views
        mcursor.moveToFirst();

        for (int i = 0; i < itemLines; i++) {
            RemoteViews subLineViews = new RemoteViews(getPackageName(),
                    R.layout.subline_item);
            for (int j = 0; j < 2; j++) {
                // build a thumbnailItem
                RemoteViews thumbnailItem = new RemoteViews(getPackageName(),
                        R.layout.bookmark_thumbnail);
                Bitmap bitmap = getBitMap(mcursor);
                thumbnailItem.setTextViewText(R.id.label, mcursor
                        .getString(Browser.HISTORY_PROJECTION_TITLE_INDEX));
                if (bitmap != null) {
                    thumbnailItem.setImageViewBitmap(R.id.thumb, bitmap);
                }

                subLineViews.addView(R.id.subline_item, thumbnailItem);

                mcursor.moveToNext();
            }
            mainViews.addView(R.id.appwidget_store, subLineViews);
        }
        return mainViews;
    }

    private Bitmap getBitMap(Cursor mcursor) {
        // Browser.HISTORY_PROJECTION_THUMBNAIL_INDEX
        byte[] data = mcursor.getBlob(7);
        if (data == null) {
            return null;
        }
        return BitmapFactory.decodeByteArray(data, 0, data.length);
    }

    @Override
    public IBinder onBind(Intent arg0) {
        // TODO Auto-generated method stub
        return null;
    }

}
