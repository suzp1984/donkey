
package com.broncho.filemanager;

import android.app.Dialog;
import android.content.Context;
import android.view.Display;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;

public class FileActionDialog extends Dialog {
    public TextView textPartial;

    public ProgressBar progressPartial;

    public TextView textAbsolute;

    public ProgressBar progressAbsolute;

    public Button buttonCancel;

    public FileActionDialog(Context context, String title) {
        super(context);
        setContentView(R.layout.fileactiondialog);
        setTitle(title);
        LayoutParams paramsDialog = getWindow().getAttributes();
        WindowManager wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
        Display display = wm.getDefaultDisplay();
        paramsDialog.width = (int)((double)display.getWidth() * 0.9);

        textPartial = (TextView)findViewById(R.id.fileActionDialogTextPartial);
        progressPartial = (ProgressBar)findViewById(R.id.fileActionDialogProgressPartial);
        textAbsolute = (TextView)findViewById(R.id.fileActionDialogTextAbsolute);
        progressAbsolute = (ProgressBar)findViewById(R.id.fileActionDialogProgressAbsolute);
        buttonCancel = (Button)findViewById(R.id.fileActionDialogButtonCancel);

    }
}
