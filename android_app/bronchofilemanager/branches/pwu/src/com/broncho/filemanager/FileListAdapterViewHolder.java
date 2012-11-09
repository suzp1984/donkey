
package com.broncho.filemanager;

import android.widget.ImageView;
import android.widget.TextView;

import com.broncho.android.widget.ViewHolder;

/**
 * @author Marc Nuri San F�lix
 */
public class FileListAdapterViewHolder extends ViewHolder<FileListAdapterEntry> {
    public ImageView icon;

    public TextView fileName;

    public TextView fileSize;

    public FileListAdapterViewHolder(ImageView icon, TextView fileName, TextView fileSize) {
        this.icon = icon;
        this.fileName = fileName;
        this.fileSize = fileSize;
    }
}
