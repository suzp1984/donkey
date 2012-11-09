package com.zhixin.zxlbs.gpsagent;

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import com.zhixin.zxlbs.utils.Log;
import com.zhixin.zxlbs.utils.SettingItem;
import com.zhixin.zxlbs.utils.XmlParser;

public class GpsAgentSettings {
    public final static String REARVIEW_MIRROR_CONFIG_PATH = "/data/etc/rearview_mirror.xml";

    private final static String REMOTE_SERVER_ADDR = "remote_server_addr";
    private final static String REMOTE_SERVER_PORT = "remote_server_port";
    private final static String LOCAL_AGENT_PORT = "local_agent_port";
    private final static String PORT_ARRAY = "port_table";

    private Map<String, String> mapPortTable;

    private String mRemoteServerAddr;
    private String mRemoteServerPort;
    private String mLocalAgentPort;

    public GpsAgentSettings(String portArray, String remoteServerAddr,
            String remoteServerPort, String localAgentPort) {
        mapPortTable = parserArray(portArray);
        mRemoteServerAddr = remoteServerAddr;
        mRemoteServerPort = remoteServerPort;
        mLocalAgentPort = localAgentPort;
    }

    private Map<String, String> parserArray(String str) {
        Map<String, String> portMap = new HashMap<String, String>();
        String[] arrayItems = str.split(";");

        for (String element : arrayItems) {
            String[] entrys = element.split(":");
            String cmd = entrys[0];
            String port = entrys[1];

            portMap.put(cmd, port);
        }
        
        return portMap;
    }

    public Map<Byte, Integer> getPortTable() {
        Map<Byte, Integer> retPortTable = new HashMap<Byte, Integer>();

        for (Object o : mapPortTable.keySet()) {
            retPortTable.put(Integer.valueOf((String) o).byteValue(), Integer
                    .valueOf(mapPortTable.get(o)));
        }
        return retPortTable;
    }

    public String getRemoteServerAddr() {
        return mRemoteServerAddr;
    }

    public Integer getRemoteServerPort() {
        return Integer.valueOf(mRemoteServerPort);
    }

    public Integer getLocalAgentPort() {
        return Integer.valueOf(mLocalAgentPort);
    }

    public static GpsAgentSettings getGpsAgentSettingsFromConfig() {
        XmlParser parser = new XmlParser(REARVIEW_MIRROR_CONFIG_PATH);
        List<SettingItem> items = parser.parseSettingItem();
        // Map<String, String> portTable = parser.parsePortTable();
        Iterator<SettingItem> iterator = items.iterator();

        String remoteServerAddr = null;
        String remoteServerPort = null;
        String localAgentPort = null;
        String portArrayStr = null;

        while (iterator.hasNext()) {
            SettingItem item = iterator.next();
            if (REMOTE_SERVER_ADDR.equals(item.getId())) {
                remoteServerAddr = item.getValue();
            } else if (REMOTE_SERVER_PORT.equals(item.getId())) {
                remoteServerPort = item.getValue();
            } else if (LOCAL_AGENT_PORT.equals(item.getId())) {
                localAgentPort = item.getValue();
                Log.d("get local_agent_port is " + localAgentPort);
            } else if (PORT_ARRAY.equals(item.getId())) {
                portArrayStr = item.getValue();
            }
        }

        return new GpsAgentSettings(portArrayStr, remoteServerAddr,
                remoteServerPort, localAgentPort);
    }
}
