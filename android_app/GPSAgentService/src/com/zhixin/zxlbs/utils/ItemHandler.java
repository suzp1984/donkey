package com.zhixin.zxlbs.utils;

import java.util.ArrayList;
import java.util.List;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

public class ItemHandler extends DefaultHandler {
    private List<SettingItem> items;
    private boolean isInRearView = false;

    public List<SettingItem> getItems() {
        return items;
    }

    @Override
    public void startDocument() throws SAXException {
        super.startDocument();
        items = new ArrayList<SettingItem>();
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
            SettingItem item = new SettingItem();
            item.setId(attributes.getValue("id"));
            item.setValue(attributes.getValue("value"));
            items.add(item);
        }

        return;
    }

}
