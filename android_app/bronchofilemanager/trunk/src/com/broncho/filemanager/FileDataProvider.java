
package com.broncho.filemanager;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.DataSetObserver;
import android.net.Uri;
import android.util.Log;
import android.widget.Toast;

import com.broncho.filemanager.BookMark.CustomMark;

public class FileDataProvider {
    private static boolean mselectEnable = false;

    private Context mContext;

    private File mCurrentDirectory;

    private ArrayList<FileListAdapterEntry> mEntryList;

    private List<File> mPathRecoder;

    private int mPathPosition;

    private boolean isTrace;

    private FileListAdapter mListAdapter;

    private FileListAdapter mViewAdapter;

    private ArrayList<File> mListClipBoard;

    private boolean mCut;

    // may be they should be private static
    public boolean canRename;

    public boolean canWrite;

    public boolean canPaste;

    public boolean canDelete;

    public int selectedFiles;

    public File selectedFile;

    public FileDataProvider(Context context) {
        mContext = context;
        mCurrentDirectory = new File("/sdcard");
        mEntryList = new ArrayList<FileListAdapterEntry>();
        mListClipBoard = new ArrayList<File>();
        mPathRecoder = new ArrayList<File>();
        mPathPosition = 0;
        isTrace = true;
        canRename = false;
        canWrite = false;
        canPaste = false;
        canDelete = false;
        selectedFile = null;
        selectedFiles = 0;
        mCut = false;

        mListAdapter = new FileListAdapter(mContext, R.layout.simple_list, mEntryList);
        mListAdapter.registerDataSetObserver(new DataSetObserver() {
            @Override
            public void onChanged() {
                super.onChanged();

                updateProperties();
            }
        });

        mViewAdapter = new FileListAdapter(mContext, R.layout.view_list, mEntryList);
        mViewAdapter.registerDataSetObserver(new DataSetObserver() {
            @Override
            public void onChanged() {
                super.onChanged();

                updateProperties();
            }
        });
    }

    public File getCurrentDirectory() {
        return this.mCurrentDirectory;
    }

    public void move() {
        copy(true);
    }

    public void copy() {
        copy(false);
    }

    private void copy(boolean isCut) {
        mListClipBoard.clear();
        if (selectedFiles > 0) {
            mCut = isCut;
            for (FileListAdapterEntry entry : mEntryList) {
                if (entry.selected) {
                    mListClipBoard.add(entry.file);
                }
            }

            notifyDataChanged();
        }
    }

    public void sort_by_size() {
        FileSizeComp comparator = new FileSizeComp();
        sortFiles(comparator);
    }

    public void sort_by_name() {
        FileNameComp comparator = new FileNameComp();
        sortFiles(comparator);
    }

    private void sortFiles(Comparator<File> comparator) {
        File[] files = mCurrentDirectory.listFiles();
        mEntryList.clear();
        Arrays.sort(files, comparator);
        int length = files.length;

        for (int i = 0; i < length; i++) {
            mEntryList.add(getFileListAdapterEntryFormFile(files[i]));
            notifyDataChanged();
        }
    }

    public void sort_by_date() {
        FileLastModifiedComp comparator = new FileLastModifiedComp();
        sortFiles(comparator);

    }

    public FileWorker paste(FileActionDialog progressDialog) {
        FileWorker ret = null;
        if (canPaste) {
            ret = new FileWorkerPaste(progressDialog, mListClipBoard, mCurrentDirectory, mCut) {

                @Override
                protected void done(Exception exception) {
                    // TODO Auto-generated method stub
                    if (exception != null) {
                        exception.printStackTrace();
                        Toast toast = Toast.makeText(mContext, "Error when pasting",
                                Toast.LENGTH_SHORT);
                        toast.show();
                    }
                    if (actionCancelled) {
                        Toast toast = Toast.makeText(mContext, "Pasting cancelled by user",
                                Toast.LENGTH_SHORT);
                        toast.show();
                    }
                    if (cutError) {
                        Toast toast = Toast.makeText(mContext,
                                "Not every cut file could be deleted", Toast.LENGTH_SHORT);
                        toast.show();
                    }
                    refresh();

                }
            };
        }

        return ret;
    }

    public FileWorker delete(FileActionDialog progressDialog) {
        FileWorker ret = null;
        if (canDelete) {
            ret = new FileWorker(progressDialog) {

                @Override
                protected void done(Exception exception) {
                    if (exception != null) {
                        exception.printStackTrace();
                        Toast toast = Toast.makeText(mContext, "Error when deleting",
                                Toast.LENGTH_SHORT);
                        toast.show();
                    }
                    refresh();

                }

                @Override
                protected void doInBackGround() throws Exception {
                    int absoluteProgress = 0;
                    int absoluteMax = selectedFiles;
                    ;
                    /*
                     * In order to prevent a stack overflow its better to create
                     * a list of pointers to files and then delete them.
                     */
                    ArrayList<File> toDeleteCollection = new ArrayList<File>();
                    for (FileListAdapterEntry entry : mEntryList) {
                        if (actionCancelled) {
                            break;
                        }
                        if (entry.selected) {
                            toDeleteCollection.clear();
                            StringBuilder absoluteMessage = new StringBuilder("Deleting selected ");
                            absoluteMessage.append(absoluteProgress + 1);
                            absoluteMessage.append(" of ");
                            absoluteMessage.append(absoluteMax);
                            publish("Caching files", 0, 1, absoluteMessage.toString(),
                                    absoluteProgress, absoluteMax);
                            absoluteProgress++;
                            gather(entry.file, toDeleteCollection, this);

                            int partialProgress = 0;
                            int partialMax = toDeleteCollection.size();
                            for (File deleteFile : toDeleteCollection) {
                                if (actionCancelled) {
                                    break;
                                }
                                partialProgress++;
                                String name = deleteFile.getName();
                                StringBuilder partialMessage = new StringBuilder("Deleting ");
                                partialMessage.append(name);
                                publish(partialMessage.toString(), partialProgress, partialMax,
                                        null, absoluteProgress, absoluteMax);
                                deleteFile.delete();
                            }
                        }
                    }
                }
            };
        }
        return ret;
    }

    private void gather(File source, ArrayList<File> cached, FileWorker worker) {
        if (worker.actionCancelled) {
            return;
        }
        if (source.isDirectory()) {
            for (File child : source.listFiles()) {
                gather(child, cached, worker);
            }
        }
        // IMPORTANT TO ADD IT AFTER ITS CHILDREN.
        // TO DELETE MUST DELETE CHILDREN FIRST.
        cached.add(source);
    }

    public FileListAdapter getListAdapter() {
        return mListAdapter;
    }

    public FileListAdapter getViewAdapter() {
        return mViewAdapter;
    }

    public void home() {
        isTrace = true;
        navigateTo(new File("/sdcard"));
    }

    public void navigateTo(File file) {
        if (file == null) {
            up();
        } else if (!file.isDirectory()) {
            // Notes: use the bellow way, it has a bug when handles the Chinese file name
            /*
            try {
              
                Intent intent = new Intent(Intent.ACTION_VIEW, Uri.fromFile(file));
                String type = MimeTypeMap.getSingleton().getMimeTypeFromExtension(
                        MimeTypeMap.getFileExtensionFromUrl(file.getCanonicalPath()));

                if (type != null) {
                    intent.setType(type);
                    intent.setDataAndType(Uri.fromFile(file), type);
                }
                mContext.startActivity(intent);             
            } catch (IOException ex) {
            } catch (ActivityNotFoundException ex) {
            }
            */
            openFile(mContext, file);
            
            return;
        } else if (file.canRead()) {
            mEntryList.clear();
            mCurrentDirectory = file;

            setPathRecoder(file);

            String path = mCurrentDirectory.getAbsolutePath();
            // add cutomtitle

            if (path.length() > 1) {
                path = path + "/";
            }
            // set the custom title here
            File[] filesArray = mCurrentDirectory.listFiles();

            // sort the filesArray
            int fileLength = filesArray.length;
            for (int it = 0; it < fileLength; it++) {
                mEntryList.add(getFileListAdapterEntryFormFile(filesArray[it]));
            }
            notifyDataChanged();
        }
    }
    
    private void openFile(Context context, File f) {
        Intent intent = new Intent();
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setAction(android.content.Intent.ACTION_VIEW);

        String type = getMIMEType(f);
        intent.setDataAndType(Uri.fromFile(f), type);
        context.startActivity(intent);
    }

    private String getMIMEType(File f) {
        String type = "";
        String fName = f.getName();

        String end = fName
                .substring(fName.lastIndexOf(".") + 1, fName.length())
                .toLowerCase();

        if (end.equals("m4a") || end.equals("mp3") || end.equals("mid")
                || end.equals("xmf") || end.equals("ogg") || end.equals("wav")) {
            type = "audio";
        } else if (end.equals("3gp") || end.equals("mp4")) {
            type = "video";
        } else if (end.equals("jpg") || end.equals("gif") || end.equals("png")
                || end.equals("jpeg") || end.equals("bmp")) {
            type = "image";
        } else if (end.equals("apk")) {
            /* android.permission.INSTALL_PACKAGES */
            type = "application/vnd.android.package-archive";
        } else {
            type = "*";
        }
        
        if (end.equals("apk")) {
        } else {
            type += "/*";
        }
        return type;
    }

    private void setPathRecoder(File file) {
        if (isTrace) {
            if (mPathPosition != mPathRecoder.size()) {
                mPathRecoder = mPathRecoder.subList(0, mPathPosition);
                Log.w("setPathRecoder", "in the first part: mPathPosition" + mPathPosition);
            }

            Log.w("mPathPosition", "  " + mPathPosition);
            if (mPathPosition > 0 && !mPathRecoder.get(mPathPosition - 1).equals(file)) {
                mPathRecoder.add(file);
                mPathPosition++;
                Log.w("mPathPosition", "in ifpart 2");
            }

            if (mPathPosition == 0) {
                mPathRecoder.add(file);
                mPathPosition++;
            }

        }
    }

    public void navigateTo(int position) {
        isTrace = true;
        navigateTo(mListAdapter.getItem(position).file);
    }

    private FileListAdapterEntry getFileListAdapterEntryFormFile(File file) {
        // TODO Auto-generated method stub

        return new FileListAdapterEntry(file, false, null);
    }

    public void up() {
        // TODO Auto-generated method stub
        if (mCurrentDirectory.getParentFile() != null) {
            isTrace = true;
            navigateTo(mCurrentDirectory.getParentFile());
        }
    }

    public void refresh() {
        isTrace = false;
        navigateTo(mCurrentDirectory);
    }

    public void selectFile(int position) {
        if (isSelectEnable()) {
            FileListAdapterEntry entry = mListAdapter.getItem(position);
            if (entry.file != null) {
                // entry.iconResource = null;
                entry.selected = !entry.selected;
                notifyDataChanged();
            }
        }
    }

    public boolean isSelectEnable() {
        return mselectEnable;
    }

    public void setSelectEnable() {
        mselectEnable = true;
    }

    public void setSelectFalse() {
        mselectEnable = false;
    }

    public void selectAll() {
        setSelect(true);
    }

    public void selectNone() {
        setSelect(false);
    }

    public void createDirectory(String directoryName) throws IOException {
        if (!canWrite) {
            throw new IOException("Permission denied!");
        }
        File newFile = new File(mCurrentDirectory.getAbsolutePath() + "/" + directoryName);
        newFile.mkdir();
    }

    public void backward() {
        if (mPathRecoder.size() > 0 && mPathPosition > 1) {
            mPathPosition--;
            isTrace = false;
            navigateTo(mPathRecoder.get(mPathPosition - 1));
        }
    }

    public void forward() {
        if (mPathRecoder.size() > mPathPosition) {
            isTrace = false;
            navigateTo(mPathRecoder.get(mPathPosition));
            mPathPosition++;
        }
    }

    private void setSelect(boolean b) {
        // TODO Auto-generated method stub
        for (FileListAdapterEntry entry : mEntryList) {
            if (entry.file != null) {
                // entry.iconResource = null;
                entry.selected = b;
                notifyDataChanged();
            }
        }

    }

    /**
	 * 
	 */
    private void notifyDataChanged() {
        mListAdapter.notifyDataSetChanged();
        mViewAdapter.notifyDataSetChanged();
    }

    private void updateProperties() {
        canRename = false;
        canWrite = mCurrentDirectory.canWrite();
        canPaste = false;
        canDelete = false;
        selectedFiles = 0;
        selectedFile = null;

        for (FileListAdapterEntry entry : mEntryList) {
            if (entry.selected) {
                selectedFiles++;
                selectedFile = entry.file;
                if (selectedFile.canWrite()) {
                    canDelete = true;
                } else {
                    canDelete = false;
                }
            }
        }

        if (selectedFiles != 1) {
            selectedFile = null;
        } else {
            if (selectedFile.canWrite()) {
                canRename = true;
            }
        }

        if (mListClipBoard.size() > 0) {
            if (canWrite) {
                canPaste = true;
            }
        }
    }

	public void addBookMark() {
		ContentValues values = new ContentValues();
		values.put(CustomMark.CREATED_DATE, System.currentTimeMillis());
		values.put(CustomMark.TITLE, mCurrentDirectory.toString());
		Uri mUri = mContext.getContentResolver().insert(CustomMark.CONTENT_URL, values);
	}
}
