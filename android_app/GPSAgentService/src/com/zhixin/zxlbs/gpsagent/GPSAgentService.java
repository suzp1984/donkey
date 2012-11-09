package com.zhixin.zxlbs.gpsagent;

import com.zhixin.zxlbs.utils.Log;

import android.app.Service;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Map;

public class GPSAgentService extends Service {

    private GpsAgentSettings settings;
    private GPSAgentThread agentThread;

    DatagramSocket mListenSocket;

    @Override
    public void onCreate() {
        super.onCreate();
        settings = GpsAgentSettings.getGpsAgentSettingsFromConfig();

        try {
            mListenSocket = new DatagramSocket(settings.getLocalAgentPort());
            Log.d("AgentServer up and running..." + "get Local Agent Port is "
                    + settings.getLocalAgentPort());
        } catch (SocketException e) {
            Log.e("Cannot create Agent listenning socket");
            e.printStackTrace();
        }
    }

    @Override
    public IBinder onBind(Intent intent) {

        return null;
    }

    @Override
    public void onStart(Intent intent, int startId) {
        Log.d("start the GPS Agent Service");
        agentThread = new GPSAgentThread();
        // agentThread.setDaemon(true);
        agentThread.start();
    }

    @Override
    public void onDestroy() {
        Log.d("destroy GPSAgentService!");
        super.onDestroy();
    }

    public class GPSAgentThread extends Thread {

        Map<Byte, Integer> mMapPortTable;

        public GPSAgentThread() {
            super("GPSAgentThread");
            mMapPortTable = settings.getPortTable();
        }

        public void run() {
            if (mListenSocket == null) {
                return;
            }

            while (true) {
                InetAddress address;
                int port;
                DatagramPacket packet;
                byte[] data = new byte[1460];
                packet = new DatagramPacket(data, data.length);
                try {
                    mListenSocket.receive(packet);
                    Log.d("receive packet!----!");
                } catch (IOException e) {
                    Log.e("Exception: in listenSocket receiver part");
                    e.printStackTrace();
                }

                address = packet.getAddress();
                port = packet.getPort();
          
                try {
                    if (address.equals(InetAddress.getByName(settings
                            .getRemoteServerAddr()))) {
                        // if address come from remote address
                        Log.d("get Packet form remote server!!--===!!");
                        if (mMapPortTable.containsKey(data[3])) {
                            // send broadcast to all local port if cmd = 0
                            if (mMapPortTable.get(data[3]) == 0) {
                                for (Integer localPort : mMapPortTable.values()) {
                                    if (localPort != 0) {
                                        packet = new DatagramPacket(data,
                                                data.length, InetAddress
                                                        .getLocalHost(),
                                                localPort);

                                        mListenSocket.send(packet);
                                        Log.d("send packet to local port");
                                    }
                                }
                            } else {
                                //send packet to special port
                                packet = new DatagramPacket(data, data.length,
                                        InetAddress.getLocalHost(),
                                        mMapPortTable.get(data[3]));

                                mListenSocket.send(packet);
                                Log.d("send packet to local port");
                            }
                        }
                    } else if (address.equals(InetAddress.getLocalHost())) {
                        // if address come from local host
                        Log.d("get packte form local host!!==--!!");

                        packet = new DatagramPacket(data, data.length,
                                InetAddress.getByName(settings
                                        .getRemoteServerAddr()), settings
                                        .getRemoteServerPort());

                        mListenSocket.send(packet);
                        Log.d("send packet to remote Server!");
                    }
                } catch (UnknownHostException e) {
                    Log.e("Exception: Address of Settings has problem");
                    e.printStackTrace();
                } catch (IOException e) {
                    Log.e("Exception: socket send fail");
                    e.printStackTrace();
                }

            }
        }
    }
}
