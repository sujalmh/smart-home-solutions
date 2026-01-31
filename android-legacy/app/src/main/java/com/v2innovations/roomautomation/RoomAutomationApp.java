package com.v2innovations.roomautomation;

/**
 * Created by virupaksha on 20-11-2016.
 */

import android.app.Application;
import android.content.Context;

import io.socket.client.IO;
import io.socket.client.Socket;

import java.net.URISyntaxException;

public class RoomAutomationApp extends Application {
    private RoomAutomationDBHelper dbHelper = null;
    private static Context context;
    private static int clientInd = 19;


    @Override
    public void onCreate() {
        super.onCreate();
        //dbHelper = new RoomAutomationDBHelper(this);
        try {

            //IO.Options opts = new IO.Options();
            //opts.forceNew = false;
            //opts.reconnection = true;
            //Socket socket = IO.socket("http://localhost", opts);

            mSocket = IO.socket("http://192.168.0.8:5000");
            //mSocket = IO.socket("http://192.168.0.6:5000",opts);
            //mSocket = IO.socket("http://192.168.1.3:5000",opts);
        } catch (URISyntaxException e) {
            throw new RuntimeException(e);
        }

    }


    public void setContext(Context context) {
        this.context = context;
        dbHelper = new RoomAutomationDBHelper(this.context);
    }

    public RoomAutomationDBHelper getDbHelper() { return dbHelper; }

    private static Socket mSocket;
    /*
    {
        try {

            IO.Options opts = new IO.Options();
            opts.forceNew = false;
            opts.reconnection = true;
            //Socket socket = IO.socket("http://localhost", opts);

            //mSocket = IO.socket("http://192.168.0.5:5000");
            mSocket = IO.socket("http://192.168.0.5:5000",opts);
            //mSocket = IO.socket("http://192.168.1.2:5000");
        } catch (URISyntaxException e) {
            throw new RuntimeException(e);
        }
    }
    */
    public Socket getSocket() {
        return mSocket;
    }
    public int getClientInd() {
        ++clientInd;
        return clientInd;
    }
}