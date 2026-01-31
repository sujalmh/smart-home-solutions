package com.v2innovations.roomautomation;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.os.AsyncTask;
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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URI;
import java.net.URISyntaxException;

import io.socket.client.Ack;
import io.socket.client.Socket;
import io.socket.emitter.Emitter;
import org.json.JSONException;
import org.json.JSONObject;


/**
 * Created by virupaksha on 24-11-2016.
 */


public class RoomAutomationCurrentStatusService extends Service {
    RoomAutomationDBHelper dbHelper = null;
    String ipAddress,portNumber,serverid,clientid,compid;
    int local = 0;
    String TAG = "RoomAutomation";
    Cursor cursor;
    private Socket mSocket;
    JSONObject obj = null;


    @Override
    public void onCreate() {
        //dbHelper = new RoomAutomationDBHelper(getApplicationContext());
        RoomAutomationApp app = (RoomAutomationApp) getApplication();
        dbHelper = app.getDbHelper();
        //cursor = dbHelper.getIPCredentials();
        /*
        if (cursor.moveToFirst()){
            do {
                ipAddress=cursor.getString(0);
                portNumber=cursor.getString(1);
            } while (cursor.moveToNext());
        }
        */
        //ipAddress = "192.168.4.1";
        /*
        cursor = dbHelper.getUser();

        if (cursor.moveToFirst()){
            try {
                // Sending an object
                obj = new JSONObject();
                obj.put("name", cursor.getString(0));
                Log.d("RoomAutomation", cursor.getString(0));
                obj.put("pword", cursor.getString(1));
                Log.d("RoomAutomation", cursor.getString(1));
                String dev = cursor.getString(2);
                Log.d("RoomAutomation",cursor.getString(2));
                obj.put("devID",dev.substring(4));
            }catch (JSONException e) {
                Log.d("RoomAutomation", e.toString());
            }
        }
        */
        portNumber = "6000";
        //RoomAutomationApp app = (RoomAutomationApp) getApplication();

        mSocket = app.getSocket();
        //mSocket.off();

        if (!mSocket.connected()) {
            mSocket.close();
            mSocket.connect();
        }

        mSocket.once(Socket.EVENT_CONNECT, new Emitter.Listener() {
            @Override
            public void call(Object... objects) {
                Cursor cursor = dbHelper.getUser();
                if (cursor.moveToFirst()){
                    try {
                        // Sending an object
                        obj = new JSONObject();
                        obj.put("name", cursor.getString(0));
                        Log.d("RoomAutomation", cursor.getString(0));
                        obj.put("pword", cursor.getString(1));
                        Log.d("RoomAutomation", cursor.getString(1));
                        String dev = cursor.getString(2);
                        Log.d("RoomAutomation",cursor.getString(2));
                        obj.put("devID",dev.substring(4));
                    }catch (JSONException e) {
                        Log.d("RoomAutomation", e.toString());
                    }
                }

                mSocket.emit("upd client", obj, new Ack() {
                    @Override
                    public void call(Object... args) {
                        //JSONObject data = (JSONObject) args[0];
                        //String message = data.toString();
                        String message = args[0].toString();
                        Log.d("RoomAutomation", message);
                    }
                });
            }
        });

        super.onCreate();
    }

    @Override
    public void onDestroy() {
        if (dbHelper != null)
            dbHelper.close();
        /*
        if (mSocket != null) {
            mSocket.disconnect();
            mSocket.close();
        }
        */
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        AlertDialog alertDialog;
        String parameterValue = "";
        String parameter = "sta";
        serverid = "";
        clientid = "";
        compid = "";

        // Get extra data included in the Intent
        clientid = intent.getStringExtra("clientid");
        serverid = intent.getStringExtra("serverid");
        ipAddress = dbHelper.getServerIP(serverid);
        Log.d("onStartCommand", "Got serverid: " + serverid);
        Log.d("onStartCommand", "Got clientid: " + clientid);
        local = intent.getIntExtra("local", 0);

        Cursor cursor = dbHelper.getAllComponents(clientid);
        /*
        alertDialog = new AlertDialog.Builder(getApplicationContext())
                .setTitle("Load Current Home Status:")
                .setCancelable(true)
                .create();

        alertDialog.setMessage("Please wait while we are loading the current home status");
        if(!alertDialog.isShowing())
        {
            alertDialog.show();
        }
        */

        if (cursor.moveToFirst()){
            do {
                clientid = cursor.getString(0);
                compid = cursor.getString(1);

                if (local == 1) {
                    parameterValue = "";
                    //clientid = cursor.getString(0);
                    //compid = cursor.getString(1);
                   // parameterValue += cursor.getString(0) + ";";
                    //parameterValue += cursor.getString(1) + ";";
                    parameterValue = clientid.substring(4) + ";" + compid;

                    // execute HTTP request
                    if (ipAddress.length() > 0 && portNumber.length() > 0) {
                        Log.d("RoomAutomation","CurrentStatus command: "+parameterValue);
                        new HttpRequestAsyncTask(
                                getApplicationContext(), parameterValue, ipAddress, portNumber, "ini", clientid, compid
                        ).execute();
                    }
                }
                else {
                    try {
                        // Sending an object
                        obj = new JSONObject();
                        obj.put("serverID", serverid.substring(4));
                        obj.put("devID", clientid.substring(4));
                        obj.put("comp", compid);
                    }catch (JSONException e) {
                        Log.d("RoomAutomation", e.toString());
                    }
                    mSocket.emit("status", obj, new Ack() {
                        @Override
                        public void call(Object... args) {
                            JSONObject data = (JSONObject) args[0];
                            String message = data.toString();
                            Log.d("RoomAutomation", message);
                        }
                    });
                    SystemClock.sleep(200);

                }

            } while (cursor.moveToNext());
        }

        if (local == 1) {
            if (ipAddress.length() > 0 && portNumber.length() > 0) {
                new HttpRequestAsyncTask(
                        getApplicationContext(), "BYE;BYE", ipAddress, portNumber, "ini", "BYE", "BYE"
                ).execute();
            }
        }
        else {
            try {
                // Sending an object
                obj = new JSONObject();
                obj.put("serverID", serverid.substring(4));
                obj.put("devID", clientid.substring(4));
                obj.put("comp", "BYE");
            }catch (JSONException e) {
                Log.d("RoomAutomation", e.toString());
            }
            mSocket.emit("status", obj, new Ack() {
                @Override
                public void call(Object... args) {
                    JSONObject data = (JSONObject) args[0];
                    String message = data.toString();
                    Log.d("RoomAutomation", message);
                }
            });

        }

        mSocket.on("staresult", onStatusResult);
/*
        Log.d(TAG, "All Room modules Status Loading Done: ");

        Cursor cursor1 = dbHelper.getAllRooms();
        int on = 0;
        if(cursor1.moveToFirst()) {
            do {
                on = 0;
                cursor = dbHelper.getRoomModules(cursor1.getString(0));
                if (cursor.moveToFirst()) {
                    do {
                        if (cursor.getInt(0) == 1) {
                            on = 1;
                            break;
                        }

                    } while (cursor.moveToNext());
                }
                if (on == 1) {
                    dbHelper.updateRoomsStatus(cursor1.getString(0), 1);
                }
                else {
                    dbHelper.updateRoomsStatus(cursor1.getString(0),0);
                }
            } while(cursor1.moveToNext());
        }

        Log.d(TAG, "Status Loading Done: ");
        Intent intent1 = new Intent("status-load-done");
        // You can also include some extra data.
        intent1.putExtra("message", "Status Loading Done!");
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent1);
*/
        return START_STICKY;
    }

    private Emitter.Listener onStatusResult = new Emitter.Listener() {
        @Override
        public void call(final Object... args) {
            JSONObject data = (JSONObject) args[0];
            String devid;
            String comp;
            int mod,stat,val;
            try {
                devid = "RSW-"+data.getString("devID");
                comp = data.getString("comp");
                mod = data.getInt("mod");
                stat = data.getInt("stat");
                val = data.getInt("val");
                if (comp.indexOf("BYE") == -1) {
                    /*
                    if ((comp.indexOf("Comp0") != -1) || (comp.indexOf("Comp1") != -1)) {
                        dbHelper.updateModuleValue(devid, comp, val);
                        dbHelper.updateModuleStatus(devid, comp, val);
                    } else {
                        if (val > 0)
                            dbHelper.updateModuleStatus(devid, comp, 1);
                        else
                            dbHelper.updateModuleStatus(devid, comp, 0);
                        dbHelper.updateModuleValue(devid, comp, val);
                    }
                    */
                    dbHelper.updateModule(devid,comp,mod,stat,val);
                }
                else {

                    Log.d(TAG, "All Room components Status Loading Done: ");
                    Intent intent1 = new Intent("status-load-done");
                    // You can also include some extra data.
                    intent1.putExtra("message", "Status Loading Done!");
                    intent1.putExtra("clientid",clientid);
                    LocalBroadcastManager.getInstance(getApplicationContext()).sendBroadcast(intent1);

                }
            } catch (JSONException e) {
                        Log.d("RoomAutomation", e.toString());
            }

        }
    };


    /**
     * Description: Send an HTTP Get request to a specified ip address and port.
     * Also send a parameter "parameterName" with the value of "parameterValue".
     * @param parameterValue the pin number to toggle
     * @param ipAddress the ip address to send the request to
     * @param portNumber the port number of the ip address
     * @param parameterName
     * @return The ip address' reply text, or an ERROR message is it fails to receive one
     */
    public String sendRequest(String parameterValue, String ipAddress, String portNumber, String parameterName) {
        String serverResponse = "ERROR";

        try {

            HttpClient httpclient = new DefaultHttpClient(); // create an HTTP client
            // define the URL e.g. http://myIpaddress:myport/?pin=13 (to toggle pin 13 for example)
            URI website = new URI("http://"+ipAddress+":"+portNumber+"/?"+"usr"+parameterName+"="+parameterValue);
            HttpGet getRequest = new HttpGet(); // create an HTTP GET object
            getRequest.setURI(website); // set the URL of the GET request
            HttpResponse response = httpclient.execute(getRequest); // execute the request
            // get the ip address server's reply
            InputStream content = null;
            content = response.getEntity().getContent();
            BufferedReader in = new BufferedReader(new InputStreamReader(
                    content
            ));
            serverResponse = in.readLine();
            // Close the connection
            content.close();
        } catch (ClientProtocolException e) {
            // HTTP error
            serverResponse = e.getMessage();
            e.printStackTrace();
        } catch (IOException e) {
            // IO error
            serverResponse = e.getMessage();
            e.printStackTrace();
        } catch (URISyntaxException e) {
            // URL syntax error
            serverResponse = e.getMessage();
            e.printStackTrace();
        }
        // return the server's reply/response text
        return serverResponse;
    }


    /**
     * An AsyncTask is needed to execute HTTP requests in the background so that they do not
     * block the user interface.
     */
    private class HttpRequestAsyncTask extends AsyncTask<Void, Void, Void> {

        // declare variables needed
        private String requestReply,ipAddress, portNumber;
        private Context context;
        private AlertDialog alertDialog;
        private String parameter;
        private String parameterValue;
        private String roomid,moduleid;

        /**
         * Description: The asyncTask class constructor. Assigns the values used in its other methods.
         * @param context the application context, needed to create the dialog
         * @param parameterValue the pin number to toggle
         * @param ipAddress the ip address to send the request to
         * @param portNumber the port number of the ip address
         */
        public HttpRequestAsyncTask(Context context, String parameterValue, String ipAddress, String portNumber, String parameter,String roomid, String moduleid)
        {
            this.context = context;
            /*
            alertDialog = new AlertDialog.Builder(this.context)
                    .setTitle("HTTP Response From IP Address:")
                    .setCancelable(true)
                    .create();
            */
            this.ipAddress = ipAddress;
            this.parameterValue = parameterValue;
            this.portNumber = portNumber;
            this.parameter = parameter;
            this.roomid = roomid;
            this.moduleid = moduleid;
        }

        /**
         * Name: doInBackground
         * Description: Sends the request to the ip address
         * @param voids
         * @return
         */
        @Override
        protected Void doInBackground(Void... voids) {
            /*
            alertDialog.setMessage("Data sent, waiting for reply from server...");
            if(!alertDialog.isShowing())
            {
                alertDialog.show();
            }
            */
            //Toast.makeText(getApplicationContext(), "Data sent, waiting for reply from server...", Toast.LENGTH_LONG).show();
            requestReply = sendRequest(parameterValue,ipAddress,portNumber, parameter);
            return null;
        }

        /**
         * Name: onPostExecute
         * Description: This function is executed after the HTTP request returns from the ip address.
         * The function sets the dialog's message with the reply text from the server and display the dialog
         * if it's not displayed already (in case it was closed by accident);
         * @param aVoid void parameter
         */
        @Override
        protected void onPostExecute(Void aVoid) {
            //int val=0;
            /*
            alertDialog.setMessage(requestReply);
            if(!alertDialog.isShowing())
            {
                alertDialog.show(); // show dialog
            }
            */

            if (!requestReply.contains("BYE")) {
                Log.d("RoomAutomation","current sta: "+requestReply);
                String[] strtok = requestReply.split(";");
                if(strtok.length > 0) {
                    String client = "RSW-" + strtok[0];
                    Log.d("RoomAutomation", "client: " + client);
                    String comp = strtok[1];
                    Log.d("RoomAutomation", "comp: " + comp);
                    int mod = Integer.parseInt(strtok[2]);
                    Log.d("RoomAutomation", "mod: " + strtok[2]);
                    int stat = Integer.parseInt(strtok[3]);
                    Log.d("RoomAutomation", "stat: " + strtok[3]);
                    int val = Integer.parseInt(strtok[4]);
                    val = (val > 1000) ? 1000 : val;
                    Log.d("RoomAutomation", "val: " + strtok[4]);
                /*
                if ((comp.indexOf("Comp0") != -1) || (comp.indexOf("Comp1") != -1)) {
                    dbHelper.updateModuleValue(client, comp, val);
                    dbHelper.updateModuleStatus(client, comp, val);
                } else {
                    if (val > 0)
                        dbHelper.updateModuleStatus(client, comp, 1);
                    else
                        dbHelper.updateModuleStatus(client, comp, 0);
                    dbHelper.updateModuleValue(client, comp, val);
                }
                */
                    dbHelper.updateModule(client, comp, mod, stat, val);
                }
            }
            else {

                Log.d(TAG, "All Room components Status Loading Done: ");
                Intent intent1 = new Intent("status-load-done");
                // You can also include some extra data.
                intent1.putExtra("message", "Status Loading Done!");
                intent1.putExtra("clientid",clientid);
                //intent1.putExtra("")
                LocalBroadcastManager.getInstance(this.context).sendBroadcast(intent1);

            }

        }

        /**
         * Name: onPreExecute
         * Description: This function is executed before the HTTP request is sent to ip address.
         * The function will set the dialog's message and display the dialog.
         */
        @Override
        protected void onPreExecute() {
            /*
            alertDialog.setMessage("Sending data to server, please wait...");
            if(!alertDialog.isShowing())
            {
                alertDialog.show();
            }
            */
            //Toast.makeText(getApplicationContext(), "Sending data to server, please wait...", Toast.LENGTH_LONG).show();
        }

    }



}

