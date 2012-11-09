package com.zhixin.zxlbs.park.util;

import java.util.ArrayList;
import java.util.List;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

public class SimpleHandler extends DefaultHandler {
    private List<Item> items;
    private boolean isInRearView = false;

    public List<Item> getItems() {
        return items;
    }

    @Override
    public void startDocument() throws SAXException {
        super.startDocument();
        items = new ArrayList<Item>();
    }

    @Override
    public void startElement(String uri, String localName, String name,
            Attributes attributes) throws SAXException {
        super.startElement(uri, localName, name, attributes);
        if (localName.equalsIgnoreCase("rearview_mirror")) {
            isInRearView = true;
            return;
        }

        if (localName.equalsIgnoreCase("item") && isInRearView == true) {
            Item item = new Item();
            item.setId(attributes.getValue("id"));
            item.setValue(attributes.getValue("value"));
            items.add(item);
        } else {
            isInRearView = false;
        }

        return;
    }

}
