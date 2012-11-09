package com.zhixin.zxlbs.park.util;

import java.io.File;
import java.util.List;

import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

public class SaxXmlParser {

    final String fileName;

    public SaxXmlParser(String fileName) {
        this.fileName = fileName;
    }

    public List<Item> parse() {
        SAXParserFactory factory = SAXParserFactory.newInstance();
        try {
            SAXParser parser = factory.newSAXParser();
            SimpleHandler handler = new SimpleHandler();
            parser.parse(getInputStream(), handler);
            return handler.getItems();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    private File getInputStream() {
        return new File(fileName);
    }
}
