package com.zhixin.zxlbs.utils;

import java.util.HashMap;
import java.util.Map;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

public class MapTableHandler extends DefaultHandler {

    private static final String ROOT_ELEMENT = "rearview_mirror";
    private static final String RECORD = "record";
    private static final String PORT_TABLE = "port_table";

    private boolean isInRearView = false;
    private boolean isInPortTable = false;

    private Map<String, String> mapTable;

    public Map<String, String> getPortTable() {
        return mapTable;
    }

    @Override
    public void startDocument() throws SAXException {
        super.startDocument();
        mapTable = new HashMap<String, String>();
    }

    @Override
    public void startElement(String uri, String localName, String qName,
            Attributes attributes) throws SAXException {
        super.startElement(uri, localName, qName, attributes);
        if (localName.equalsIgnoreCase(ROOT_ELEMENT)) {
            isInRearView = true;
            return;
        }

        if (localName.equalsIgnoreCase("array") && isInRearView == true) {
            if (attributes.getValue("name").equalsIgnoreCase(PORT_TABLE)) {
                isInPortTable = true;
                return;
            }
        }

        if (localName.equalsIgnoreCase(RECORD) && isInRearView == true
                && isInPortTable == true) {
            String cmd = attributes.getValue("cmd");
            String port = attributes.getValue("port");

            mapTable.put(cmd, port);
            return;
        }
    }

    @Override
    public void endElement(String uri, String localName, String qName)
            throws SAXException {
        if (localName.equalsIgnoreCase(PORT_TABLE)) {
            isInPortTable = false;
            return;
        }

        if (localName.equalsIgnoreCase(ROOT_ELEMENT)) {
            isInRearView = false;
            return;
        }
    }

}
