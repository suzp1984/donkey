package com.topwise.aftersale;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;


import com.topwise.aftersale.utils.Log;


public class AfterSaleManager {
    private int provice_count;
    
    private List<AfterSale> mAfterSaleList;
    private List<String> mProvices;

    public AfterSaleManager() {
        mProvices = new ArrayList<String>();
        
        mAfterSaleList = new ArrayList<AfterSale>();
        mAfterSaleList.addAll(new XmlConfigAfterSale().getAftersales());
        
        Iterator<AfterSale> iterator = mAfterSaleList.iterator();
        while (iterator.hasNext()) {
            
            AfterSale after_sale = iterator.next();
            String provice = after_sale.getProvice();
            if (!mProvices.contains(provice)) {
                mProvices.add(provice);
            }   
        }
        
        provice_count = mProvices.size();       
        //Log.e("provice count is " + provice_count);
    }
    
    public List<String> getProvices() {
        return mProvices;
    }
    
    public List<AfterSale> getAfterSales(String provice) {
        List<AfterSale> lists = new ArrayList<AfterSale>();
        Iterator<AfterSale> iter = mAfterSaleList.iterator();
        
        if (mProvices.contains(provice)) {
            while (iter.hasNext()) {
                AfterSale aftersale = iter.next();
                if (aftersale.getProvice().equals(provice)) {
                    lists.add(aftersale);
                }
            }
            
        }
        
        return lists;
    }
    
}
