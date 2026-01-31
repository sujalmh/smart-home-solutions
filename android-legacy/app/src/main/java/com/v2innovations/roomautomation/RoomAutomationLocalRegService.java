package com.v2innovations.roomautomation;

import android.app.Service;
import android.content.Intent;
import android.database.Cursor;
import android.os.IBinder;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.ServerSocket;
import java.net.Socket;

/**
 * Created by virupaksha on 23-01-2017.
 */
public class RoomAutomationLocalRegService extends Service {
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
        if (serverSocket != null)
            try {
                serverSocket.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        if (serverThread != null)
            serverThread.interrupt();
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        serverid = intent.getStringExtra("serverid");
        this.serverThread = new Thread(new ServerThread());
        this.serverThread.start();
        return START_STICKY;
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
                serverSocket = new ServerSocket(6000);
                //} catch (IOException e) {
                //e.printStackTrace();
                //}

                //while (true) {
                while (!serverThread.isInterrupted()) {
                    System.out.println("Waiting for client request");
                    try {
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
                            if (msg.contains("drg=")) {
                                String data = msg.substring(4);
                                String[] strtok = data.split(";");
                                if (strtok.length > 0) {
                                    String client = "RSW-" + strtok[0];
                                    String ip = strtok[1];
                                    dbHelper.insertClient(serverid, client, strtok[0], ip);
                                }
                            } else if (msg.contains("dst=")) {
                                String data = msg.substring(4);
                                String[] strtok = data.split(";");
                                if (strtok.length > 0) {
                                    String client = "RSW-" + strtok[0];
                                    if (!client.contains("END")) {
                                        String comp = strtok[1];
                                        int mod = Integer.parseInt(strtok[2]);
                                        int stat = Integer.parseInt(strtok[3]);
                                        int val = (Integer.parseInt(strtok[4]) > 1000) ? 1000 : Integer.parseInt(strtok[4]);
                                        dbHelper.updateModule(client, comp, mod, stat, val);
                                    } else {
                                        socket.close();
                                        serverSocket.close();
                                        Intent intent1 = new Intent("register-done");
                                        intent1.putExtra("register", data);
                                        LocalBroadcastManager.getInstance(getApplicationContext()).sendBroadcast(intent1);
                                        serverThread.interrupt();
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
                        socket.close();
                        //terminate the server if client sends exit request
                    } catch (IOException e) {
                        Log.d("RoomAutomation", "RoomRegisterService" + e.toString());
                        e.printStackTrace();
                    }

                }
            } catch (IOException e) {
                Log.d("RoomAutomation", "RoomRegisterService" + e.toString());
                e.printStackTrace();
            } finally {
                    try {
                        if (serverSocket != null)
                        serverSocket.close();
                    } catch (IOException e) {
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
