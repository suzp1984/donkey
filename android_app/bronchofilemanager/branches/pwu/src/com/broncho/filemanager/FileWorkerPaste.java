
package com.broncho.filemanager;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;
import java.util.ArrayList;
import java.util.LinkedHashMap;

public abstract class FileWorkerPaste extends FileWorker {
    private ArrayList<File> list;

    private File currentDirectory;

    private boolean cut;

    public boolean cutError;

    public FileWorkerPaste(FileActionDialog progressDialog, ArrayList<File> list,
            File currentDirectory, boolean cut) {
        super(progressDialog);
        this.list = list;
        this.currentDirectory = currentDirectory;
        this.cut = cut;
        cutError = false;
    }

    @Override
    protected void doInBackGround() throws Exception {
        int absoluteProgress = 0;
        int absoluteMax = list.size();
        /*
         * In order to prevent a stack overflow its better to create a list of
         * pointers to files and then delete them.
         */
        LinkedHashMap<File, File> toPasteCollection = new LinkedHashMap<File, File>();
        for (File clipboardOrigin : list) {
            if (actionCancelled) {
                break;
            }
            toPasteCollection.clear();
            StringBuilder absoluteMessage = new StringBuilder("Pasting clipboard ");
            absoluteMessage.append(absoluteProgress + 1);
            absoluteMessage.append(" of ");
            absoluteMessage.append(absoluteMax);
            publish("Caching files", 0, 1, absoluteMessage.toString(), absoluteProgress,
                    absoluteMax);
            absoluteProgress++;
            // APPEND -copy if exists
            String clipboardOriginName = clipboardOrigin.getName();
            File clipBoardDestination = new File(currentDirectory.getAbsolutePath() + "/"
                    + clipboardOriginName);
            while (clipBoardDestination.exists()) {
                clipBoardDestination = new File(clipBoardDestination.getAbsolutePath() + "-copy");
            }
            gather(clipboardOrigin, clipBoardDestination, toPasteCollection);
            // Created pointer for current clipboard file
            // Begin paste
            int partialProgress = 0;
            File[] keyFiles = toPasteCollection.keySet().toArray(new File[0]);
            int partialMax = keyFiles.length;
            for (File keyFile : keyFiles) {
                if (actionCancelled) {
                    break;
                }
                partialProgress++;
                File destination = toPasteCollection.get(keyFile);
                String name = destination.getName();
                StringBuilder partialMessage = new StringBuilder("Pasting ");
                partialMessage.append(name);
                publish(partialMessage.toString(), partialProgress, partialMax, null,
                        absoluteProgress, absoluteMax);
                copy(keyFile, destination);
            }
            if (cut) {
                partialProgress = 0;
                for (int it = partialMax - 1; it > -1; it--) {
                    partialProgress++;
                    String name = keyFiles[it].getName();
                    StringBuilder partialMessage = new StringBuilder("Deleting ");
                    partialMessage.append(name);
                    publish(partialMessage.toString(), partialProgress, partialMax, null,
                            absoluteProgress, absoluteMax);
                    if (!keyFiles[it].delete()) {
                        cutError = true;
                    }
                }
            }
        }
    }

    private void gather(File source, File destination, LinkedHashMap<File, File> cached) {
        if (actionCancelled) {
            return;
        }
        cached.put(source, destination);
        if (source.isDirectory()) {
            for (String child : source.list()) {
                gather(new File(source, child), new File(destination, child), cached);
            }

        }
    }

    private void copy(File source, File destination) throws IOException, FileNotFoundException {
        if (source.isDirectory()) {
            destination.mkdirs();
        } else {
            FileChannel in = new FileInputStream(source).getChannel();
            destination.createNewFile();
            FileChannel out = new FileOutputStream(destination).getChannel();
            if (source.length() != 0l) {
                // IF File is of size 0, this throws
                // Exception
                // http://issues.apache.org/jira/browse/HARMONY-6315
                MappedByteBuffer buf = in.map(MapMode.READ_ONLY, 0, in.size());
                out.write(buf);
            }
            if (in != null)
                in.close();
            if (out != null)
                out.close();
        }
    }
}
