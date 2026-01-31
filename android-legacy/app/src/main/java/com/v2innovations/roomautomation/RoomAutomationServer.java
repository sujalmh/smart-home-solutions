package com.v2innovations.roomautomation;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.IBinder;
import android.os.SystemClock;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.app.AlertDialog;
import android.util.Log;

import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.ObjectInputStream;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.URI;
import java.net.URISyntaxException;


/**
 * Created by virupaksha on 16-12-2016.
 */
public class RoomAutomationServer extends Service {
    RoomAutomationDBHelper dbHelper = null;
    String ipAddress, portNumber, serverid, clientid, compid;
    int local = 0;
    String TAG = "RoomAutomation";
    Cursor cursor;

    private ServerSocket serverSocket;
    //Handler updateConversationHandler;
    Thread serverThread = null;
    private BufferedReader input;

    @Override
    public void onCreate() {
        //dbHelper = new RoomAutomationDBHelper(getApplicationContext());
        RoomAutomationApp app = (RoomAutomationApp) getApplication();
        dbHelper = app.getDbHelper();
        ///updateConversationHandler = new Handler();
        // this.serverThread = new Thread(new ServerThread());
        //this.serverThread.start();

        super.onCreate();
    }

    @Override
    public void onDestroy() {
        if (dbHelper != null)
            dbHelper.close();
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        this.serverThread = new Thread(new ServerThread());
        this.serverThread.start();
        return START_STICKY;
    }

    //added by viru on 1-5-2017
    @Override
    public boolean stopService(Intent name) {
        try {
            serverSocket.close();
            serverThread.interrupt();
        }catch(IOException e){
            Log.d("RoomAutomationServer","stopService"+e.toString());
        }
        return super.stopService(name);
    }

    /*
        @Override
        protected void onStop() {
            super.onStop();
            try {
                serverSocket.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    */
    class ServerThread implements Runnable {

        public void run() {

            Socket socket = null;
            try {
                serverSocket = new ServerSocket(); // <-- create an unbound socket first
                serverSocket.setReuseAddress(true);
                serverSocket.bind(new InetSocketAddress(6000)); // <-- now bind it
                //serverSocket = new ServerSocket(6000);
            } catch (IOException e) {
                e.printStackTrace();
            }

            while(true){
                System.out.println("Waiting for client request");
                try {
                    //added by viru on 1-5-2017
                    if (serverSocket.isClosed()) {
                        Log.d("RoomAutomationServer","ServerSocket closed");
                        //break;
                        //serverSocket = new ServerSocket(); // <-- create an unbound socket first
                        //serverSocket.setReuseAddress(true);
                        //serverSocket.bind(new InetSocketAddress(6000)); // <-- now bind it
                    }
                    else {
                        //creating socket and waiting for client connection
                        socket = serverSocket.accept();
                        //read from socket to ObjectInputStream object
                        //ObjectInputStream ois = new ObjectInputStream(socket.getInputStream());
                        input = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                        //convert ObjectInputStream object to String
                        String msg = (String) input.readLine();//input.readObject();
                        //System.out.println("Message Received: " + msg);
                        if (msg != null) {
                            Log.d("RoomAutomation", "message from esp: " + msg);
                            if (msg.contains("sta=") || msg.contains("res=")) {
                                String data = msg.substring(4);
                                String[] strtok = data.split(";");
                                if (strtok.length > 0) {
                                    String client = "RSW-" + strtok[0];
                                    String comp = strtok[1];
                                    int mod = Integer.parseInt(strtok[2]);
                                    int stat = Integer.parseInt(strtok[3]);
                                    int val = (Integer.parseInt(strtok[4]) > 1000) ? 1000 : Integer.parseInt(strtok[4]);
                                    dbHelper.updateModule(client, comp, mod, stat, val);
                                    Intent intent1 = new Intent("status-update");
                                    intent1.putExtra("status", data);
                                    LocalBroadcastManager.getInstance(getApplicationContext()).sendBroadcast(intent1);
                                }
                            }
                        }
                    }

                /*
                //create ObjectOutputStream object
                ObjectOutputStream oos = new ObjectOutputStream(socket.getOutputStream());
                //write object to Socket
                oos.writeObject("Hi Client "+message);
                //close resources
                */
                    //ois.close();
                    //oos.close();
                    //socket.close();
                    //terminate the server if client sends exit request
                }catch (IOException e) {
                    Log.d("RoomAutomation","RoomSocketServer"+e.toString());
                    e.printStackTrace();
                }

            }
            //System.out.println("Shutting down Socket server!!");
            //close the ServerSocket object
            //serverSocket.close();
        }


            /*
            while (!Thread.currentThread().isInterrupted()) {
                //SystemClock.sleep(1000);
                try {

                    socket = serverSocket.accept();

                    CommunicationThread commThread = new CommunicationThread(socket);
                    new Thread(commThread).start();

                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            */
        }
    }
/*
    class CommunicationThread implements Runnable {

        private Socket clientSocket;

        private BufferedReader input;

        public CommunicationThread(Socket clientSocket) {

            this.clientSocket = clientSocket;

            try {

                this.input = new BufferedReader(new InputStreamReader(this.clientSocket.getInputStream()));

            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        public void run() {

            while (!Thread.currentThread().isInterrupted()) {

                try {

                    String read = input.readLine();

                    updateConversationHandler.post(new updateUIThread(read));

                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }

    }

    class updateUIThread implements Runnable {
        private String msg;

        public updateUIThread(String str) {
            this.msg = str;
        }

        @Override
        public void run() {
            //text.setText(text.getText().toString()+"Client Says: "+ msg + "\n");
            //Intent intent1 = new Intent("status-update");
            // You can also include some extra data.
            //intent1.putExtra("status", msg);
            //intent1.putExtra("clientid",clientid);
            //intent1.putExtra("")
            if (msg != null) {
                Log.d("RoomAutomation", "message from esp: " + msg);
                if (msg.contains("sta=") || msg.contains("res=")) {
                    String data = msg.substring(4);
                    String[] strtok = data.split(";");
                    String client = "RSW-" + strtok[0];
                    String comp = strtok[1];
                    int mod = Integer.parseInt(strtok[2]);
                    int stat = Integer.parseInt(strtok[3]);
                    int val = Integer.parseInt(strtok[4]);
                    dbHelper.updateModule(client, comp, mod, stat, val);
                    Intent intent1 = new Intent("status-update");
                    intent1.putExtra("status", data);
                    LocalBroadcastManager.getInstance(getApplicationContext()).sendBroadcast(intent1);
                }
            }
        }
    }

}
*/
