
package com.broncho.filemanager;

import java.io.File;

public class FileListAdapterEntry {
    public Integer iconResource;

    public File file;

    public boolean selected;

    public FileListAdapterEntry(File file, boolean selected, Integer iconResource) {
        this.file = file;
        this.selected = selected;
        this.iconResource = iconResource;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((file == null) ? 0 : file.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        FileListAdapterEntry other = (FileListAdapterEntry)obj;
        if (file == null) {
            if (other.file != null)
                return false;
        } else if (!file.equals(other.file)) {
            return false;
        }

        return true;
    }
}
