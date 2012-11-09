
package com.broncho.android.widget;

import java.util.ArrayList;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

public abstract class FDataAdapter<T, V extends ViewHolder<T>> extends BaseAdapter {

    protected Context mContext;

    private ArrayList<T> mListItems = new ArrayList<T>();

    private int mViewId;

    private LayoutInflater mLayoutInflater;

    public FDataAdapter(Context context, int viewid, ArrayList<T> listItem) {
        mContext = context;
        mListItems = listItem;
        mViewId = viewid;
        mLayoutInflater = LayoutInflater.from(context);
    }

    public int getCount() {
        return mListItems.size();
    }

    public T getItem(int position) {
        return mListItems.get(position);
    }

    public long getItemId(int position) {
        return position;
    }

    public View getView(int position, View convertView, ViewGroup parent) {

        V holder;

        // when convertView not null, we can reuse it
        if (convertView == null) {
            convertView = mLayoutInflater.inflate(mViewId, null);
            holder = createHolder(convertView);
            convertView.setTag(holder);
        } else {
            holder = (V)convertView.getTag();
        }

        holder.data = getItem(position);
        // call user's implementation
        bindHolder(holder);

        return convertView;
    }

    protected abstract V createHolder(View v);

    protected abstract void bindHolder(V h);
}
