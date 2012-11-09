
package com.broncho.filemanager;

import java.io.File;
import java.util.Comparator;

/**
 * @author suzhenxing
 */
public class FileNameComp implements Comparator<File> {
    public int compare(File file, File compared) {
        if (file.isDirectory() && compared.isDirectory()) {
            return file.getName().compareTo(compared.getName());
        } else if (file.isDirectory() && !compared.isDirectory()) {
            return -1;
        } else if (!file.isDirectory() && compared.isDirectory()) {
            return 1;
        } else {
            return file.getName().compareTo(compared.getName());
        }
    }

}
