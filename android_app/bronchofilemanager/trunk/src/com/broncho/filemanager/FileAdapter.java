
package com.broncho.filemanager;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class FileAdapter extends BaseAdapter {

    // private LayoutInflater mInflater;
    private Context mContext;

    public FileAdapter(Context c) {
        mContext = c;
        // mInflater = LayoutInflater.from(c);
    }

    public int getCount() {
        // TODO Auto-generated method stub
        return mStr.length;
    }

    public Object getItem(int position) {
        // TODO Auto-generated method stub
        return position;
    }

    public long getItemId(int position) {
        // TODO Auto-generated method stub
        return 0;
    }

    public View getView(int position, View convertView, ViewGroup parent) {
        // TODO Auto-generated method stub
        // convertView = mInflater.inflate(R.layout.list_item, null);
        TextView text;
        if (convertView == null) {
            text = new TextView(mContext);
        } else {
            text = (TextView)convertView;
        }

        text.setText(mStr[position]);
        return text;
    }

    private String[] mStr = {
            "test1", "test2", "test3", "test4"
    };
}
