package com.broncho.filemanager;

import java.sql.SQLException;
import java.util.HashMap;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;
import android.text.TextUtils;

import com.broncho.filemanager.BookMark.CustomMark;
import com.broncho.filemanager.util.Log;

public class BookMarkProvider extends ContentProvider {

	// private static final String TAG = "BookMarkProvider";
	private static final String DATABASE_NAME = "bookmark.db";
	private static final int DATABASE_VERSION = 2;
	private static final String BOOKMARK_TABLE_NAME = "bookmark";

	private static final UriMatcher sUriMatcher;
	
	private static final int BOOKMARK = 1;
	private static final int BOOKMARK_ID = 2;
	
	private static HashMap<String, String> sBookMarkProjectionMap;
	
	static {
		sUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
		sUriMatcher.addURI(BookMark.AUTHORITY, "bookmark", BOOKMARK);
		sUriMatcher.addURI(BookMark.AUTHORITY, "bookmark/#", BOOKMARK_ID);
		
		sBookMarkProjectionMap = new HashMap<String, String>();
		sBookMarkProjectionMap.put(CustomMark._ID, CustomMark._ID);
		sBookMarkProjectionMap.put(CustomMark.TITLE, CustomMark.TITLE);
		sBookMarkProjectionMap.put(CustomMark.CREATED_DATE, CustomMark.CREATED_DATE);
		sBookMarkProjectionMap.put(CustomMark.MODIFIED_DATE, CustomMark.MODIFIED_DATE);
	}
	/**
	 * This class helps open, create and upgrade the databases file.
	 */
	private static class DatabaseHelper extends SQLiteOpenHelper {

		DatabaseHelper(Context context) {
			super(context, DATABASE_NAME, null, DATABASE_VERSION);
		}

		@Override
		public void onCreate(SQLiteDatabase db) {
			db.execSQL("CREATE TABLE " + BOOKMARK_TABLE_NAME + " ("
					+ CustomMark._ID + " INTEGER PRIMARY KEY,"
					+ CustomMark.TITLE + " TEXT," + CustomMark.CREATED_DATE
					+ " INTEGER," + CustomMark.MODIFIED_DATE + " INTEGER"
					+ " );");

		}

		@Override
		public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
			Log.w("Upgrading database from version " + oldVersion + " to "
					+ newVersion + " , which will destroy all old data");
			db.execSQL("DROP TABLE IF EXISTS boomark");
			onCreate(db);

		}

	}

	private DatabaseHelper mOpenHelper;

	@Override
	public int delete(Uri uri, String selection, String[] selectionArgs) {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public String getType(Uri uri) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public Uri insert(Uri uri, ContentValues initvalues) {
		if(sUriMatcher.match(uri) != BOOKMARK) {
			throw new IllegalArgumentException("Unknown URI" + uri);
		}
		
		ContentValues values;
		if(initvalues != null) {
			values = new ContentValues(initvalues);
		} else {
			values = new ContentValues();
		}
		
		Long now = Long.valueOf(System.currentTimeMillis());
		
		if(values.containsKey(CustomMark.CREATED_DATE) == false) {
			values.put(CustomMark.CREATED_DATE, now);
		}
		
		if(values.containsKey(CustomMark.MODIFIED_DATE) == false) {
			values.put(CustomMark.MODIFIED_DATE, now);
		}
		
		if(values.containsKey(CustomMark.TITLE) == false) {
			values.put(CustomMark.TITLE, "");
		}
		
		SQLiteDatabase db = mOpenHelper.getWritableDatabase();
		Long rowId = db.insert(BOOKMARK_TABLE_NAME, CustomMark.BOOKMARK, values);
		if(rowId > 0) {
			Uri noteUri = ContentUris.withAppendedId(CustomMark.CONTENT_URL, rowId);
			getContext().getContentResolver().notifyChange(noteUri, null);
			return noteUri;
		}
		
		try {
			throw new SQLException("Failed to insert row into " + uri);
		} catch (SQLException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return null;
	}

	@Override
	public boolean onCreate() {
		mOpenHelper = new DatabaseHelper(getContext());
		return true;
	}

	@Override
	public Cursor query(Uri uri, String[] projection, String selection,
			String[] selectionArgs, String sortOrder) {
		SQLiteQueryBuilder qb = new SQLiteQueryBuilder();
		qb.setTables(BOOKMARK_TABLE_NAME);
		
		switch(sUriMatcher.match(uri)) {
		case BOOKMARK:
			qb.setProjectionMap(sBookMarkProjectionMap);
			break;
		case BOOKMARK_ID:
			qb.setProjectionMap(sBookMarkProjectionMap);
			break;
		default:
			throw new IllegalArgumentException("Unknown URI " + uri);
		}
		
		String orderBy;
		if(TextUtils.isEmpty(sortOrder)) {
			orderBy = BookMark.CustomMark.DEFAULT_SORT_ORDER;
		} else {
			orderBy = sortOrder;
		}
		
		SQLiteDatabase db = mOpenHelper.getReadableDatabase();
		Cursor c = db.query(BOOKMARK_TABLE_NAME, projection, selection, 
				selectionArgs, null, null, orderBy);
		c.setNotificationUri(getContext().getContentResolver(), uri);
		
		return c;
	}

	@Override
	public int update(Uri uri, ContentValues values, String selection,
			String[] selectionArgs) {
		// TODO Auto-generated method stub
		return 0;
	}

}
