package com.broncho.sensor;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

public class CalibrationActivity extends Activity implements SensorEventListener {
    /** Called when the activity is first created. */
	SensorManager mgr;
	static private String CALIBRATE_PATH = "/data/etc/m_sensor";
	private float[] mData = {0,0,0,1,1,1};
	
	private ExtremityMonitor mValue = new ExtremityMonitor();
	
	private File mFile;
	
	OnClickListener mListener = new OnClickListener() {
		
		public void onClick(View v) {
		
			float tmp = (mValue.getMaxY() - mValue.getMinY()) /
			(mValue.getMaxX() - mValue.getMinX());
			if(tmp > 1) {
				mData[3] = tmp;
			} else {
				mData[4] = 1/tmp;
			}
			
			
			mData[0] = ((mValue.getMaxX() - mValue.getMinX()) / 2 - mValue.getMaxX()) * mData[3];
			
			mData[1] = ((mValue.getMaxY() - mValue.getMinY()) / 2 - mValue.getMaxY()) * mData[4];

			
			Button calZ = (Button) findViewById(R.id.button);
			calZ.setOnClickListener(mCalZListener);
			TextView text = (TextView) findViewById(R.id.tip);
			text.setText(R.string.step2);
			mValue.reset();
		
		}
		
	};
	
	OnClickListener mCalZListener = new OnClickListener() {
		
		public void onClick(View v) {
			//cal z axi
			
			mData[2] = (mValue.getMaxZ() - mValue.getMinZ())/2 - mValue.getMaxZ();
			
			try {
				//FileOutputStream fos = openFileOutput(CALIBRATE_PATH, MODE_WORLD_WRITEABLE);
				PrintStream p = new PrintStream(mFile);
				for(int i=0; i < mData.length; i++) {
					p.print(mData[i]);
					p.print(" ");
				}
				p.close();
				//fos.close();
				Runtime.getRuntime().exec("chmod 666 "+CALIBRATE_PATH);
			} catch (FileNotFoundException e) {
				
				e.printStackTrace();
			} catch (IOException e) {
				
				e.printStackTrace();
			}
			
			finish();
		}
	};
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
         mFile = new File(CALIBRATE_PATH);
         
         if(mFile.exists()) {
        	 mFile.delete();
         }
         
         Button button = (Button)findViewById(R.id.button);
         button.setOnClickListener(mListener);
         
         mgr = (SensorManager) getSystemService(SENSOR_SERVICE);
         
    }
    
    @Override
    public void onResume() {
    	super.onResume();
    	
    	Sensor sensor = mgr.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
    	mgr.registerListener(this, sensor, SensorManager.SENSOR_DELAY_NORMAL);
    
    }

    @Override
    public void onPause() {
    	super.onPause();
    	mgr.unregisterListener(this);
    }
    
	public void onAccuracyChanged(Sensor sensor, int accuracy) {
	
		
	}

	public void onSensorChanged(SensorEvent event) {
		mValue.updateX(event.values[0]);
		mValue.updateY(event.values[1]);
		mValue.updateZ(event.values[2]);
		
	
	}
}