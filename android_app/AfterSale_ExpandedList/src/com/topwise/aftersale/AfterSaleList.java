package com.topwise.aftersale;

import java.util.ArrayList;
import java.util.List;

public abstract class AfterSaleList {

    protected List<AfterSale> mAftersales;
    
    public AfterSaleList() {
        mAftersales = new ArrayList<AfterSale>();
    }
    
    abstract List<AfterSale> getAftersales();
}
