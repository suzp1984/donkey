package com.broncho.filemanager;

import android.net.Uri;
import android.provider.BaseColumns;

public final class BookMark {
	public static final String AUTHORITY = "com.broncho.filemanager.bookmark";
	
	//this class cannot be instantiated
	private BookMark() {}
	
	public static final class CustomMark implements BaseColumns {
		//this class cannot be instantiated
		private CustomMark() {}
		
		/**
		 * the content:// style URL for this table
		 */
		public static final Uri CONTENT_URL = Uri.parse("content://"+AUTHORITY+"/bookmark");
		
		/**
		 * The default sort order for this table
		 */
		public static final String DEFAULT_SORT_ORDER = "modified DESC";
		
		/**
		 * The title of this bookmark
		 * <p>Type: TEXT</p>	
		 */
		public static final String TITLE = "title";
		
		/**
		 * The bookmark itself
		 * <p>Type: TEXT</p>
		 */
		public static final String BOOKMARK = "bookmark";
		
		/**
		 * The timestamp for when the note was created
		 * <p>Type: Integer</p>
		 */
		public static final String CREATED_DATE = "created";
		
		/**
		 * The timestamp for when the note was last modified
		 * <p>Type: INTEGER</p>
		 */
		public static final String MODIFIED_DATE = "modified";
		
	}
}
