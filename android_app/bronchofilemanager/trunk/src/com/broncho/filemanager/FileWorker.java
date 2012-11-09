
package com.broncho.filemanager;

import android.os.Handler;
import android.view.View;
import android.view.View.OnClickListener;

public abstract class FileWorker {
    private FileActionDialog progressDialog;

    private Thread workerThread;

    private Runnable workerRunnable;

    private Exception workerException;

    private Handler futureTaskHandler;

    private Runnable futureTaskRunnable;

    public boolean actionCancelled;

    public FileWorker(FileActionDialog progressDialog) {
        this.progressDialog = progressDialog;
        progressDialog.setCancelable(false);
        progressDialog.buttonCancel.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                cancel();

            }
        });
        progressDialog.progressPartial.setIndeterminate(false);
        progressDialog.progressAbsolute.setIndeterminate(false);
        actionCancelled = false;
        workerException = null;
        futureTaskHandler = new Handler();
        workerRunnable = new Runnable() {

            public void run() {
                try {
                    doInBackGround();
                } catch (Exception e) {
                    FileWorker.this.workerException = e;
                }
                futureTaskHandler.post(futureTaskRunnable);
            }
        };
        workerThread = new Thread(workerRunnable);
        futureTaskRunnable = new Runnable() {

            public void run() {
                done(FileWorker.this.workerException);
                FileWorker.this.progressDialog.dismiss();
            }
        };
    }

    public void execute() {
        workerThread.start();
        progressDialog.show();
    }

    protected abstract void doInBackGround() throws Exception;

    protected abstract void done(Exception exception);

    public void publish(final String partialMessage, final Integer partialProgress,
            final Integer partialMax, final String absoluteMessage, final Integer absoluteProgress,
            final Integer absoluteMax) {
        futureTaskHandler.post(new Runnable() {

            public void run() {
                process(partialMessage, partialProgress, partialMax, absoluteMessage,
                        absoluteProgress, absoluteMax);

            }
        });
    }

    private void cancel() {
        actionCancelled = true;
    }

    private void process(String partialMessage, Integer partialProgress, Integer partialMax,
            String absoluteMessage, Integer absoluteProgress, Integer absoluteMax) {
        if (partialMessage != null) {
            progressDialog.textPartial.setText(partialMessage);
        }
        if (partialProgress != null) {
            progressDialog.progressPartial.setProgress(partialProgress.intValue());
        }
        if (partialMax != null) {
            progressDialog.progressPartial.setMax(partialMax.intValue());
        }
        if (absoluteMessage != null) {
            progressDialog.textAbsolute.setText(absoluteMessage);
        }
        if (absoluteProgress != null) {
            progressDialog.progressAbsolute.setProgress(absoluteProgress.intValue());
        }
        if (absoluteMax != null) {
            progressDialog.progressAbsolute.setMax(absoluteMax.intValue());
        }
    }
}
