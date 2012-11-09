
package com.broncho.filemanager;

import java.io.File;
import java.util.Comparator;

/*
 * @author suzhenxing
 */
public class FileLastModifiedComp implements Comparator<File> {
    public int compare(File file, File compared) {
        return new Long(file.lastModified()).compareTo(new Long(compared.length()));
    }
}
