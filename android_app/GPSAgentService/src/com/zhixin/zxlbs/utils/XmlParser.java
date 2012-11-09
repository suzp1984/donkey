package com.zhixin.zxlbs.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.List;
import java.util.Map;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;
import org.xml.sax.helpers.XMLReaderFactory;


public class XmlParser {
    final String fileName;
    ItemHandler itemHandler = new ItemHandler();
    //MapTableHandler mapTableHandler = new MapTableHandler();
    
    public XmlParser(String fileName) {
        this.fileName = fileName;
        //System.setProperty("org.xml.sax.driver", "com.example.xml.SAXDriver");
    }
    
    public List<SettingItem> parseSettingItem() {
        
        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            SAXParser sp = spf.newSAXParser();
            //XMLReader reader = sp.getXMLReader();
            //reader.setContentHandler(itemHandler);
            
            InputSource source = new InputSource(new 
                    FileInputStream(new File(fileName)));
            //reader.parse(source);
            sp.parse(source, itemHandler);
        } catch (SAXException e) {
            
            e.printStackTrace();
        } catch (FileNotFoundException e) {
            
            e.printStackTrace();
        } catch (IOException e) {
            
            e.printStackTrace();
        } catch (ParserConfigurationException e) {
            
            e.printStackTrace();
        }
        
        return itemHandler.getItems();    
    }
   
    /*
    public Map<String, String> parsePortTable() {
        
        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            SAXParser sp = spf.newSAXParser();
            //XMLReader reader = sp.getXMLReader();
            
            //XMLReader reader = XMLReaderFactory.createXMLReader();
            
            //reader.setContentHandler(mapTableHandler);
            // TODO may be setErrorHandler needed
            InputSource source = new InputSource(new 
                    FileInputStream(new File(fileName)));
            
            //reader.parse(source);
            sp.parse(source, mapTableHandler);
        } catch (SAXException e) {
            
            e.printStackTrace();
        } catch (FileNotFoundException e) {
            
            e.printStackTrace();
        } catch (IOException e) {
            
            e.printStackTrace();
        } catch (ParserConfigurationException e) {
            
            e.printStackTrace();
        }
        
        return mapTableHandler.getPortTable();
    }  */
}
