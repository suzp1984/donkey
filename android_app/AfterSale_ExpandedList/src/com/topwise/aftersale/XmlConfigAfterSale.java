package com.topwise.aftersale;

import java.util.List;

import android.os.Environment;

import com.topwise.aftersale.utils.SaxXmlParser;

public class XmlConfigAfterSale extends AfterSaleList {

    private static final String XML_CONFIGER = "/etc/after_sales.xml";
    
    public XmlConfigAfterSale() {
        super();
        
        SaxXmlParser xmlParser = new SaxXmlParser(
                Environment.getRootDirectory() + XML_CONFIGER);
        
        mAftersales = xmlParser.parse();
    }
    
    @Override
    public List<AfterSale> getAftersales() {
        
        return mAftersales;
    }

}
