package com.topwise.aftersale;

import com.topwise.aftersale.utils.Log;

import android.app.Activity;
import android.os.Bundle;
import android.view.Gravity;
//import android.view.LayoutInflater;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.AnimationSet;
import android.view.animation.LayoutAnimationController;
import android.view.animation.TranslateAnimation;
import android.widget.AbsListView;
import android.widget.BaseExpandableListAdapter;
import android.widget.ExpandableListView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class AfterSaleActivity extends Activity {

    ExpandableListView mExpandableView;
    private AfterSaleManager mAfterSaleManager;
    private LayoutAnimationController mAnimaterControl;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mAfterSaleManager = new AfterSaleManager();
        mExpandableView = (ExpandableListView) findViewById(
                R.id.expendableView);

        MyExpandableAdapter adapter = new MyExpandableAdapter(this);
        
        mExpandableView.setAdapter(adapter);
        
        AnimationSet set = new AnimationSet(true);
        
        Animation animation = new AlphaAnimation(0.0f, 1.0f);
        animation.setDuration(50);
        set.addAnimation(animation);
        
        animation = new TranslateAnimation(
                Animation.RELATIVE_TO_SELF, 0.0f, Animation.RELATIVE_TO_SELF, 0.0f,
                Animation.RELATIVE_TO_SELF, -1.0f, Animation.RELATIVE_TO_SELF, 0.0f
                );
        animation.setDuration(100);
        set.addAnimation(animation);
        
        mAnimaterControl = new LayoutAnimationController(set, 0.5f);
        mExpandableView.setLayoutAnimation(mAnimaterControl);
        mExpandableView.setTextFilterEnabled(true);
    }

    public class MyExpandableAdapter extends BaseExpandableListAdapter {

        Activity activity;

        public MyExpandableAdapter(Activity act) {
            activity = act;
        }

        public Object getChild(int groupPosition, int childPosition) {
            return mAfterSaleManager.getAfterSales(mAfterSaleManager.getProvices().
                    get(groupPosition)).get(childPosition);
        }

        public long getChildId(int groupPosition, int childPosition) {

            return childPosition;
        }

        public View getChildView(int groupPosition, int childPosition,
                boolean isLastChild, View convertView, ViewGroup parent) {
            AfterSale aftersale = mAfterSaleManager.getAfterSales(mAfterSaleManager.getProvices().
                    get(groupPosition)).get(childPosition);
        
            return getMyChildView(aftersale);
        }

        public int getChildrenCount(int groupPosition) {
            int count =  mAfterSaleManager.getAfterSales(mAfterSaleManager.
                    getProvices().get(groupPosition)).size();
            //Log.e("groupPosition: " + groupPosition + "; getChildrenCount is " + count);
            return count;
        }

        public Object getGroup(int groupPosition) {
            return mAfterSaleManager.getProvices().get(groupPosition);
        }

        public int getGroupCount() {
            int count = mAfterSaleManager.getProvices().size();
            //Log.e("getGroupCount is " + count);
            
            return count;
        }

        public long getGroupId(int groupPosition) {

            return groupPosition;
        }

        public View getGroupView(int groupPosition, boolean isExpanded,
                View convertView, ViewGroup parent) {
            String str = mAfterSaleManager.getProvices().get(groupPosition);
            
            return getGenericView(str);
        }

        public boolean hasStableIds() {
            // TODO Auto-generated method stub
            return false;
        }

        public boolean isChildSelectable(int groupPosition, int childPosition) {

            return true;
        }

        public TextView getGenericView(String string) {
            AbsListView.LayoutParams layoutParams = new AbsListView.LayoutParams(
                    ViewGroup.LayoutParams.FILL_PARENT, 64);
            TextView text = new TextView(activity);
            text.setLayoutParams(layoutParams);
            text.setGravity(Gravity.CENTER_VERTICAL | Gravity.LEFT);
            text.setPadding(36, 0, 0, 0);
            text.setTextSize(20);
            //text.setTypeface();
            text.setText(string);

            return text;
        }
        
        
        public LinearLayout getMyChildView(AfterSale aftersale) {
            LayoutInflater inflater = (LayoutInflater) activity.
                    getSystemService(LAYOUT_INFLATER_SERVICE);
            
            LinearLayout childview = (LinearLayout) inflater.
                    inflate(R.layout.childview, null);
            TextView address = (TextView)childview.findViewById(R.id.address);
            address.setText(aftersale.getAddress());
            
            TextView company = (TextView)childview.findViewById(R.id.company);
            company.setText(aftersale.getCompany());
            
            TextView tel = (TextView) childview.findViewById(R.id.tel);
            tel.setText(aftersale.getPhone());
            childview.setLayoutAnimation(mAnimaterControl);
                    
            return childview;            
        }

    }
}
