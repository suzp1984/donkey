
package com.broncho.filemanager;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;

import android.content.Context;
import android.content.res.Resources;
import android.text.format.Formatter;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.broncho.android.widget.FDataAdapter;

public class FileListAdapter extends FDataAdapter<FileListAdapterEntry, FileListAdapterViewHolder> {

    private HashMap<String, Integer> mimeTypes;

    public FileListAdapter(Context context, int viewid, ArrayList listItem) {
        super(context, viewid, listItem);
        
        initMimeTypes();
    }

    private void initMimeTypes() {
        
        mimeTypes = new HashMap<String, Integer>();
        Resources res = mContext.getResources();

        for (String extension : res.getStringArray(R.array.audio)) {
            mimeTypes.put(extension, R.drawable.file_audio);
        }

        for (String extension : res.getStringArray(R.array.image)) {
            mimeTypes.put(extension, R.drawable.file_image);
        }

        for (String extension : res.getStringArray(R.array.pack)) {
            mimeTypes.put(extension, R.drawable.file_pack);
        }

        for (String extension : res.getStringArray(R.array.video)) {
            mimeTypes.put(extension, R.drawable.file_video);
        }

        for (String extension : res.getStringArray(R.array.web)) {
            mimeTypes.put(extension, R.drawable.file_web);
        }
    }

    @Override
    protected FileListAdapterViewHolder createHolder(View v) {

        ImageView image = (ImageView)v.findViewById(R.id.imageIcon);
        TextView textFileName = (TextView)v.findViewById(R.id.textFileName);
        TextView textFileSize = (TextView)v.findViewById(R.id.textFileSize);
        return new FileListAdapterViewHolder(image, textFileName, textFileSize);
    }

    @Override
    protected void bindHolder(FileListAdapterViewHolder h) {

        FileListAdapterEntry data = h.data;
        File f = data.file;

        if (data.iconResource == null) {
            data.iconResource = getIcon(data);
        }

        h.icon.setImageResource(data.iconResource);
        h.fileName.setText(f.getName());
        h.fileSize.setText(Formatter.formatFileSize(mContext, f.length()));
        
        // why invoke this function twice ?
        //Log.d("filename", f.getName());        
        //if (f.isDirectory()) h.fileSize.setVisibility(View.INVISIBLE);

        if (data.selected) {
            h.fileName.setTextColor(mContext.getResources().getColor(R.color.select_red));
        } else {
            h.fileName.setTextColor(mContext.getResources().getColor(R.color.white));
        }

        return;

    }

    private Integer getIcon(FileListAdapterEntry data) {

        int ret = -1;
        File f = data.file;
        String[] fileName = f.getName().split("\\.");

        int arrayLength = fileName.length;

        if (f.isDirectory()) {
            ret = R.drawable.folder;
        } else {
            ret = R.drawable.file_text;

            if (arrayLength > 1) {
                Integer value = mimeTypes.get(fileName[arrayLength - 1]);
                if (value != null) {
                    ret = value.intValue();
                }
            }
        }
        return ret;
    }
}
