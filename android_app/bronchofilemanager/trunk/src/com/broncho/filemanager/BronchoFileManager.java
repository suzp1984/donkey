package com.broncho.filemanager;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ContentUris;
import android.content.Context;
import android.content.DialogInterface;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.GridView;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.SimpleCursorAdapter;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ViewFlipper;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;

import com.broncho.filemanager.BookMark.CustomMark;

public class BronchoFileManager extends Activity {

	private final static int MENU_ITEM_FLIPPER = 1;

	private final static String _TAG = "BronchoFileManager";

	private ViewFlipper mFileViewFlipper;

	private FileListView mFileListView;

	private FileDataProvider mDataProvider;

	private static final int DIALOG_edit = 1;

	private static final int DIALOG_sort = 2;

	private static final int DIALOG_navigation = 3;

	private static final String[] PROJECTION = new String[] { CustomMark._ID,
			CustomMark.TITLE, };

	private static final int COLUMN_INDEX_NOTE = 1;

	private static final Integer[] mImageContain = { R.drawable.menu_copy,
			R.drawable.menu_move, R.drawable.menu_paste,
			R.drawable.menu_delete, R.drawable.menu_select_all };

	private static final String[] mTextContain = { "Copy", "Move", "Paste",
			"Delete", "Select_all" };

	private static final Integer[] mSortImageContain = {
			R.drawable.sort_by_name, R.drawable.sort_by_size,
			R.drawable.sort_by_date, R.drawable.sort_by_type };

	private static final String[] mSortTextContain = { "Sort by name",
			"Sort by size", "Sort by date", "Sort by type" };

	@Override
	protected Dialog onCreateDialog(int id) {
		switch (id) {
		case DIALOG_edit: {
			AlertDialog.Builder builder;
			AlertDialog alertDialog;

			Context mContext = getApplicationContext();
			LayoutInflater inflater = (LayoutInflater) mContext
					.getSystemService(LAYOUT_INFLATER_SERVICE);
			View layout = inflater.inflate(R.layout.edit,
					(ViewGroup) findViewById(R.id.edit_root));

			ListView editListView = (ListView) layout
					.findViewById(R.id.edit_list);
			ArrayList<HashMap<String, Object>> listItem = new ArrayList<HashMap<String, Object>>();
			for (int i = 0; i < 5; i++) {
				HashMap<String, Object> map = new HashMap<String, Object>();
				map.put("ImageView", mImageContain[i]);
				map.put("TextView", mTextContain[i]);
				listItem.add(map);
			}

			SimpleAdapter listItemAdapter = new SimpleAdapter(this, listItem,
					R.layout.list_item,
					new String[] { "ImageView", "TextView" }, new int[] {
							R.id.ImageView01, R.id.TextView01 });

			editListView.setAdapter(listItemAdapter);
			editListView.setOnItemClickListener(new OnItemClickListener() {

				public void onItemClick(AdapterView<?> adapterView, View view,
						int position, long id) {
					// TODO Auto-generated method stub
					switch (position) {
					case 0:
						copy();
						break;
					case 1:
						move();
						break;
					case 2:
						paste();
						break;
					case 3:
						delete();
						break;
					default:
						break;
					}
					dismissDialog(DIALOG_edit);

				}
			});

			builder = new AlertDialog.Builder(BronchoFileManager.this);
			builder.setView(layout);
			alertDialog = builder.create();
			return alertDialog;
		}
		case DIALOG_sort: {
			AlertDialog.Builder builder;
			AlertDialog alertDialog;

			Context mContext = getApplicationContext();
			LayoutInflater inflater = (LayoutInflater) mContext
					.getSystemService(LAYOUT_INFLATER_SERVICE);
			View layout = inflater.inflate(R.layout.sort,
					(ViewGroup) findViewById(R.id.sort_root));

			ListView sortListView = (ListView) layout
					.findViewById(R.id.sort_list);
			ArrayList<HashMap<String, Object>> listItem = new ArrayList<HashMap<String, Object>>();
			for (int i = 0; i < 4; i++) {
				HashMap<String, Object> map = new HashMap<String, Object>();
				map.put("ImageView", mSortImageContain[i]);
				map.put("TextView", mSortTextContain[i]);
				listItem.add(map);
			}

			SimpleAdapter listItemAdapter = new SimpleAdapter(this, listItem,
					R.layout.list_item_sort, new String[] { "ImageView",
							"TextView" }, new int[] { R.id.ImageView02,
							R.id.TextView02 });

			sortListView.setAdapter(listItemAdapter);
			sortListView.setOnItemClickListener(new OnItemClickListener() {

				public void onItemClick(AdapterView<?> adapterView, View view,
						int position, long id) {
					// TODO Auto-generated method stub
					switch (position) {
					case 0:
						mDataProvider.sort_by_name();
						break;
					case 1:
						mDataProvider.sort_by_size();
						break;
					case 2:
						mDataProvider.sort_by_date();
						break;
					case 3:
						mDataProvider.sort_by_name();
						break;
					default:
						break;
					}
					dismissDialog(DIALOG_sort);

				}
			});

			builder = new AlertDialog.Builder(BronchoFileManager.this);
			builder.setView(layout);
			alertDialog = builder.create();
			return alertDialog;
		}
		case DIALOG_navigation: {
			AlertDialog.Builder builder;
			AlertDialog alertDialog;

			Context mContext = getApplicationContext();
			LayoutInflater inflater = (LayoutInflater) mContext
					.getSystemService(LAYOUT_INFLATER_SERVICE);

			View layout = inflater.inflate(R.layout.navigation,
					(ViewGroup) findViewById(R.id.navigation_root));
			ListView navigationListView = (ListView) layout
					.findViewById(R.id.navigation_list);
			Cursor cursor = managedQuery(CustomMark.CONTENT_URL, PROJECTION,
					null, null, CustomMark.DEFAULT_SORT_ORDER);

			SimpleCursorAdapter adapter = new SimpleCursorAdapter(this,
					R.layout.list_item_navigation, cursor,
					new String[] { CustomMark.TITLE },
					new int[] { android.R.id.text1 });

			navigationListView.setAdapter(adapter);
			navigationListView
					.setOnItemClickListener(new OnItemClickListener() {
						public void onItemClick(AdapterView<?> adapterView,
								View view, int position, long id) {
							Uri uri = ContentUris.withAppendedId(
									CustomMark.CONTENT_URL, id);
							Cursor bookMarkCursor = managedQuery(uri,
									PROJECTION, null, null, null);
							bookMarkCursor.moveToPosition(position);
							String bookMark = bookMarkCursor
									.getString(COLUMN_INDEX_NOTE);
							File file = new File(bookMark);
							if (file != null) {
								mDataProvider.navigateTo(new File(bookMark));
								setCustomTitle();
							}

							dismissDialog(DIALOG_navigation);
						}
					});

			builder = new AlertDialog.Builder(BronchoFileManager.this);
			builder.setView(layout);
			alertDialog = builder.create();
			return alertDialog;
		}
		}
		return null;

	}

	private void delete() {
		// TODO Auto-generated method stub
		FileActionDialog dialog = new FileActionDialog(this, "Deleting...");
		mDataProvider.delete(dialog).execute();
	}

	private void select_all() {
		// TODO Auto-generated method stub
		mDataProvider.selectAll();
	}

	private void paste() {
		// TODO Auto-generated method stub
		FileActionDialog dialog = new FileActionDialog(this, "Pasting...");
		mDataProvider.paste(dialog).execute();
	}

	private void copy() {
		// TODO Auto-generated method stub
		mDataProvider.copy();
	}

	private void move() {
		// TODO Auto-generated method stub
		mDataProvider.move();
	}

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		requestWindowFeature(Window.FEATURE_CUSTOM_TITLE);
		setContentView(R.layout.main);
		getWindow().setFeatureInt(Window.FEATURE_CUSTOM_TITLE,
				R.layout.custom_title);

		initComponents();

		initButton();
	}

	/**
	 * 
	 */
	private void initButton() {
		Button homeButton = (Button) findViewById(R.id.home);
		homeButton.setOnClickListener(new OnClickListener() {

			public void onClick(View v) {
				// TODO Auto-generated method stub
				mDataProvider.home();
				setCustomTitle();
			}
		});

		Button parentButton = (Button) findViewById(R.id.parent);
		parentButton.setOnClickListener(new OnClickListener() {

			public void onClick(View v) {
				// TODO Auto-generated method stub
				mDataProvider.up();
				setCustomTitle();
			}
		});

		Button mutisellectButton = (Button) findViewById(R.id.mutisellect);
		mutisellectButton.setOnClickListener(new OnClickListener() {

			public void onClick(View v) {
				// TODO Auto-generated method stub
				muti_select();
			}

		});

		Button editButton = (Button) findViewById(R.id.edit);
		editButton.setOnClickListener(new OnClickListener() {

			public void onClick(View v) {
				// TODO Auto-generated method stub
				showDialog(DIALOG_edit);
			}
		});

		Button createButton = (Button) findViewById(R.id.create);
		createButton.setOnClickListener(new OnClickListener() {

			public void onClick(View v) {
				// TODO Auto-generated method stub
				createDirectory();
			}

		});

		Button backwardButton = (Button) findViewById(R.id.backward);
		backwardButton.setOnClickListener(new OnClickListener() {

			public void onClick(View v) {
				// TODO Auto-generated method stub
				mDataProvider.backward();
				setCustomTitle();
			}
		});

		Button forwardButton = (Button) findViewById(R.id.forward);
		forwardButton.setOnClickListener(new OnClickListener() {

			public void onClick(View v) {
				// TODO Auto-generated method stub
				mDataProvider.forward();
				setCustomTitle();
			}
		});

		Button sortButton = (Button) findViewById(R.id.sort);
		sortButton.setOnClickListener(new OnClickListener() {

			public void onClick(View v) {
				// TODO Auto-generated method stub
				showDialog(DIALOG_sort);
			}
		});

		Button refreshButton = (Button) findViewById(R.id.refresh);
		refreshButton.setOnClickListener(new OnClickListener() {

			public void onClick(View v) {
				mDataProvider.refresh();
				setCustomTitle();
			}
		});

		Button viewflipButton = (Button) findViewById(R.id.viewflip);
		viewflipButton.setOnClickListener(new OnClickListener() {

			public void onClick(View v) {
				// TODO Auto-generated method stub
				fileViewFlipper();
			}
		});
	}

	private void createDirectory() {
		// TODO Auto-generated method stub
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setTitle("New Folder");
		builder.setMessage("Folder name");
		final EditText input = new EditText(this);
		builder.setView(input);
		builder.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) {
				try {
					mDataProvider.createDirectory(input.getText().toString());
				} catch (IOException e) {
					Toast toast = Toast.makeText(BronchoFileManager.this, e
							.getMessage(), Toast.LENGTH_SHORT);
					toast.show();
				}
				mDataProvider.refresh();
			}
		});

		builder.setNegativeButton("Cancel",
				new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						dialog.cancel();
					}
				});

		builder.show();

	}

	/**
	 * 
	 */
	private void muti_select() {
		if (mDataProvider.isSelectEnable()) {
			mDataProvider.setSelectFalse();
			mDataProvider.selectNone();
			Toast.makeText(BronchoFileManager.this, "Muti-select mode is OFF",
					Toast.LENGTH_SHORT).show();
		} else {
			mDataProvider.setSelectEnable();
			Toast.makeText(BronchoFileManager.this, "Muti-select mode is ON",
					Toast.LENGTH_SHORT).show();
		}
	}

	/**
	 * @param position
	 */
	private void navagateTo(int position) {
		mDataProvider.navigateTo(position);
		setCustomTitle();
	}

	private void setCustomTitle() {
		TextView titleText = (TextView) findViewById(R.id.title_right_text);

		File aFile = mDataProvider.getCurrentDirectory();
		if (aFile.canRead()) {
			titleText.setText(aFile.toString());
		}
	}

	/**
	 * 
	 */
	private void initComponents() {
		// initTestSimpleList();

		mDataProvider = new FileDataProvider(this);

		FileListView listview = (FileListView) findViewById(R.id.brlistview);
		listview.setAdapter(mDataProvider.getListAdapter());
		listview.setOnItemClickListener(new OnItemClickListener() {

			public void onItemClick(AdapterView<?> adapterView, View view,
					int position, long id) {
				navagateTo(position);
			}

		});

		listview.setOnItemLongClickListener(new OnItemLongClickListener() {

			public boolean onItemLongClick(AdapterView<?> adapterView,
					View view, int position, long id) {
				mDataProvider.selectFile(position);
				return true;
			}
		});

		// scroll bar setting
		findViewById(R.id.scrollview1).setHorizontalScrollBarEnabled(false);

		GridView gridview = (GridView) findViewById(R.id.brgridview);
		gridview.setAdapter(mDataProvider.getViewAdapter());
		gridview.setOnItemClickListener(new OnItemClickListener() {

			public void onItemClick(AdapterView<?> adapterView, View view,
					int position, long id) {
				navagateTo(position);
			}

		});

		gridview.setOnItemLongClickListener(new OnItemLongClickListener() {

			public boolean onItemLongClick(AdapterView<?> adapterView,
					View view, int position, long id) {
				mDataProvider.selectFile(position);
				return true;
			}
		});

		mFileViewFlipper = (ViewFlipper) findViewById(R.id.brflipper);

		mDataProvider.home();
		setCustomTitle();

	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// int NONE = Menu.NONE;
		// menu.add(NONE, MENU_ITEM_FLIPPER, NONE, R.string.flipper);

		// return super.onCreateOptionsMenu(menu);
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.option_menu, menu);

		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// switch (item.getItemId()) {
		// case MENU_ITEM_FLIPPER:
		// fileViewFlipper();
		// return true;
		// }
		// return super.onOptionsItemSelected(item);
		switch (item.getItemId()) {
		case R.id.menu_multi_select:
			muti_select();
			return true;
		case R.id.menu_edit:
			showDialog(DIALOG_edit);
			return true;
		case R.id.menu_sort:
			return true;
		case R.id.menu_refresh:
			mDataProvider.refresh();
			return true;
		case R.id.menu_bookmark:
			mDataProvider.addBookMark();
			return true;
		case R.id.menu_create:
			createDirectory();
			return true;
		case R.id.menu_navigation:
			showDialog(DIALOG_navigation);
			return true;
		case R.id.menu_about:
			return true;
		case R.id.menu_exit:
			finish();
			return true;
		default:
			break;
		}

		return false;
	}

	private void fileViewFlipper() {
		mFileViewFlipper.showNext();
	}

	// private void initComponents() {

	// }

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			new AlertDialog.Builder(BronchoFileManager.this).setIcon(
					android.R.drawable.ic_dialog_alert).setTitle(
					getString(R.string.exit_alert_dialog_title)).setMessage(
					getString(R.string.exit_alert_dialog_message))
					.setPositiveButton(
							getString(R.string.exit_alert_dialog_ok),
							new DialogInterface.OnClickListener() {
								public void onClick(DialogInterface dialog,
										int which) {
									BronchoFileManager.this.finish();
								}
							}).setNegativeButton(
							getString(R.string.exit_alert_dialog_cancel),
							new DialogInterface.OnClickListener() {
								public void onClick(DialogInterface dialog,
										int whichButton) {
									// User clicked cancel so do some stuff
								}
							}).show();

			return true;
		}

		return super.onKeyDown(keyCode, event);
	}

}
