package com.v2innovations.roomautomation;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.wifi.ScanResult;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.SystemClock;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;

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
import java.util.ArrayList;
import java.util.List;


import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.Toast;

import io.socket.client.Ack;
import io.socket.client.Socket;
import io.socket.emitter.Emitter;
import org.json.JSONException;
import org.json.JSONObject;

public class SwitchActivity extends AppCompatActivity implements View.OnClickListener {
    public final static String PREF_IP = "PREF_IP_ADDRESS";
    public final static String PREF_PORT = "PREF_PORT_NUMBER";
    // declare buttons and text inputs
    private Button buttonComp0,buttonComp1,buttonComp2,buttonTestStart,buttonTestStop;
    private SeekBar comp0SeekBar,comp1SeekBar;
    Switch comp0Switch,comp1Switch,comp2Switch;
    //private Spinner comp5Spinner;
    private Socket mSocket;
    JSONObject obj = null;
    int local = 0;
    String clientID = "";
    String serverID = "";
    RoomAutomationDBHelper dbHelper = null;
    String parameterValue = "";
    // get the ip address
    String ipAddress = "192.168.4.1";
    // get the port number
    String portNumber = "6000";
    boolean userChanged = false;
    int startTest = 0;
    TestStub testStub = null;
    Thread testThread = null;

    private EditText editTextIPAddress, editTextPortNumber;
    // shared preferences objects used to save the IP address and port so that the user doesn't have to
    // type them next time he/she opens the app.
    SharedPreferences.Editor editor;
    SharedPreferences sharedPreferences;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.switch_layout1);
        //Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        //setSupportActionBar(toolbar);


        sharedPreferences = getSharedPreferences("HTTP_HELPER_PREFS", Context.MODE_PRIVATE);
        editor = sharedPreferences.edit();

        RoomAutomationApp app = (RoomAutomationApp) getApplication();
        dbHelper = app.getDbHelper();

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

        Intent i = getIntent();
        local = i.getIntExtra("local", 0);
        serverID = (String ) i.getStringExtra("serverid");
        clientID = (String ) i.getStringExtra("clientid");

        //dbHelper = new RoomAutomationDBHelper(getApplicationContext());
        //RoomAutomationApp app = (RoomAutomationApp) getApplication();
        //dbHelper = app.getDbHelper();

        testStub = new TestStub();
        // assign buttons
        buttonComp0 = (Button)findViewById(R.id.comp0_btn);
        buttonComp1 = (Button)findViewById(R.id.comp1_btn);
        buttonComp2 = (Button)findViewById(R.id.comp2_btn);
        buttonTestStart = (Button)findViewById(R.id.teststart_btn);
        buttonTestStop = (Button)findViewById(R.id.teststop_btn);

        ipAddress = dbHelper.getServerIP(serverID);
        Log.d("RoomAutomation","clientid: "+clientID);
        Log.d("RoomAutomation","comp0 stat: "+dbHelper.getModuleStatus(clientID,"Comp0"));
        if(dbHelper.getModuleStatus(clientID,"Comp0") == 1) {
            buttonComp0.setText("ON");
        }
        else {
            buttonComp0.setText("OFF");
        }
        Log.d("RoomAutomation","comp1 stat: "+dbHelper.getModuleStatus(clientID,"Comp1"));
        if(dbHelper.getModuleStatus(clientID,"Comp1") == 1) {
            buttonComp1.setText("ON");
        }
        else {
            buttonComp1.setText("OFF");
        }

        Log.d("RoomAutomation","comp2 stat: "+dbHelper.getModuleStatus(clientID,"Comp2"));
        if(dbHelper.getModuleStatus(clientID,"Comp2") == 1) {
            buttonComp2.setText("ON");
        }
        else {
            buttonComp2.setText("OFF");
        }

        comp0Switch = (Switch)findViewById(R.id.ma_switch0);

        if(dbHelper.getModuleMode(clientID, "Comp0") == 1) {
            comp0Switch.setChecked(true);
            comp0Switch.setText("A");
        }
        else {
            comp0Switch.setChecked(false);
            comp0Switch.setText("M");
        }

        comp1Switch = (Switch)findViewById(R.id.ma_switch1);

        if(dbHelper.getModuleMode(clientID,"Comp1") == 1) {
            comp1Switch.setChecked(true);
            comp1Switch.setText("A");
        }
        else {
            comp1Switch.setChecked(false);
            comp1Switch.setText("M");
        }

        comp2Switch = (Switch)findViewById(R.id.ma_switch2);

        if(dbHelper.getModuleMode(clientID,"Comp2") == 1) {
            comp2Switch.setChecked(true);
            comp2Switch.setText("A");
        }
        else {
            comp2Switch.setChecked(false);
            comp2Switch.setText("M");
        }

        // set button listener (this class)
        buttonComp0.setOnClickListener(this);
        buttonComp1.setOnClickListener(this);
        buttonComp2.setOnClickListener(this);
        buttonTestStart.setOnClickListener(this);
        buttonTestStop.setOnClickListener(this);

        comp0SeekBar = (SeekBar)findViewById(R.id.comp0_sb);
        comp0SeekBar.setProgress((1000 - dbHelper.getModuleValue(clientID, "Comp0"))/200);
        comp0SeekBar.setMax(5);
        //comp0SeekBar.incrementProgressBy(100);
        comp0SeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            int progressChanged = 0;

            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                int seekLevel;
                progressChanged = progress * 200;
                seekLevel = 1000 - progressChanged;
                //SystemClock.sleep(1000);
                if (fromUser) {
                    if (local == 1) {
                        parameterValue = clientID.substring(4) + ";Comp0;" + String.valueOf(dbHelper.getModuleMode(clientID, "Comp0")) + ";" + String.valueOf(dbHelper.getModuleStatus(clientID, "Comp0")) + ";" + String.valueOf(seekLevel) + ";";
                        // execute HTTP request
                        if (ipAddress.length() > 0 && portNumber.length() > 0) {
                            new HttpRequestAsyncTask(
                                    getApplicationContext(), parameterValue, ipAddress, portNumber, "cmd"
                            ).execute();
                        }

                    } else {
                        //scket-io code
                        try {
                            // Sending an object
                            obj = new JSONObject();
                            obj.put("serverID", serverID.substring(4));
                            obj.put("devID", clientID.substring(4));
                            obj.put("comp", "Comp0");
                            obj.put("mod", dbHelper.getModuleMode(clientID, "Comp0"));
                            obj.put("stat", dbHelper.getModuleStatus(clientID, "Comp0"));
                            obj.put("val", seekLevel);
                        } catch (JSONException e) {
                            Log.d("RoomAutomation", e.toString());
                        }
                        mSocket.emit("command", obj, new Ack() {
                            @Override
                            public void call(Object... args) {
                                JSONObject data = (JSONObject) args[0];
                                String message = data.toString();
                                Log.d("RoomAutomation", message);
                            }
                        });

                    }
                }
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
                /*
                int seekLevel;
                //seekLevel = progressChanged / 10;
                seekLevel = 1000 - progressChanged;
                if (local == 1) {
                    parameterValue = clientID.substring(4) + ";Comp0;" + String.valueOf(dbHelper.getModuleMode(clientID,"Comp0"))+";"+ String.valueOf(dbHelper.getModuleStatus(clientID,"Comp0"))+";"+String.valueOf(seekLevel)+";";
                    // execute HTTP request
                    if (ipAddress.length() > 0 && portNumber.length() > 0) {
                        new HttpRequestAsyncTask(
                                getApplicationContext(), parameterValue, ipAddress, portNumber, "cmd"
                        ).execute();
                    }

                } else {
                    //scket-io code
                    try {
                        // Sending an object
                        obj = new JSONObject();
                        obj.put("devID", clientID.substring(4));
                        obj.put("comp", "Comp0");
                        obj.put("mod", dbHelper.getModuleMode(clientID,"Comp0"));
                        obj.put("stat", dbHelper.getModuleStatus(clientID, "Comp0"));
                        obj.put("val",seekLevel);
                    }catch (JSONException e) {
                        Log.d("RoomAutomation", e.toString());
                    }
                    mSocket.emit("command", obj, new Ack() {
                        @Override
                        public void call(Object... args) {
                            JSONObject data = (JSONObject) args[0];
                            String message = data.toString();
                            Log.d("RoomAutomation", message);
                        }
                    });

                }
                */
            }
        });

        comp1SeekBar = (SeekBar)findViewById(R.id.comp1_sb);
        comp1SeekBar.setProgress((1000 - dbHelper.getModuleValue(clientID, "Comp1"))/200);
        comp1SeekBar.setMax(5);
        //comp1SeekBar.incrementProgressBy(100);

        comp1SeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            int progressChanged = 0;

            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                progressChanged = progress * 200;
                int seekLevel;
                seekLevel = 1000 - progressChanged;
                //SystemClock.sleep(1000);
                if(fromUser) {
                    if (local == 1) {
                        parameterValue = clientID.substring(4) + ";Comp1;" + String.valueOf(dbHelper.getModuleMode(clientID, "Comp1")) + ";" + String.valueOf(dbHelper.getModuleStatus(clientID, "Comp1")) + ";" + String.valueOf(seekLevel) + ";";
                        // execute HTTP request
                        if (ipAddress.length() > 0 && portNumber.length() > 0) {
                            new HttpRequestAsyncTask(
                                    getApplicationContext(), parameterValue, ipAddress, portNumber, "cmd"
                            ).execute();
                        }

                    } else {
                        //scket-io code
                        try {
                            // Sending an object
                            obj = new JSONObject();
                            obj.put("serverID", serverID.substring(4));
                            obj.put("devID", clientID.substring(4));
                            obj.put("comp", "Comp1");
                            obj.put("mod", dbHelper.getModuleMode(clientID, "Comp1"));
                            obj.put("stat", dbHelper.getModuleStatus(clientID, "Comp1"));
                            obj.put("val", seekLevel);
                        } catch (JSONException e) {
                            Log.d("RoomAutomation", e.toString());
                        }
                        mSocket.emit("command", obj, new Ack() {
                            @Override
                            public void call(Object... args) {
                                JSONObject data = (JSONObject) args[0];
                                String message = data.toString();
                                Log.d("RoomAutomation", message);
                            }
                        });

                    }
                }

            }

            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
                /*
                int seekLevel;
                seekLevel = progressChanged / 10;
                if (local == 1) {
                    parameterValue = clientID.substring(4) + ";Comp1;" + String.valueOf(dbHelper.getModuleMode(clientID,"Comp1"))+";"+ String.valueOf(dbHelper.getModuleStatus(clientID, "Comp1"))+";"+String.valueOf(seekLevel)+";";
                    // execute HTTP request
                    if (ipAddress.length() > 0 && portNumber.length() > 0) {
                        new HttpRequestAsyncTask(
                                getApplicationContext(), parameterValue, ipAddress, portNumber, "cmd"
                        ).execute();
                    }

                } else {
                    //scket-io code
                    try {
                        // Sending an object
                        obj = new JSONObject();
                        obj.put("devID", clientID.substring(4));
                        obj.put("comp", "Comp1");
                        obj.put("mod", dbHelper.getModuleMode(clientID, "Comp1"));
                        obj.put("stat", dbHelper.getModuleStatus(clientID,"Comp1"));
                        obj.put("val",seekLevel);
                    }catch (JSONException e) {
                        Log.d("RoomAutomation", e.toString());
                    }
                    mSocket.emit("command", obj, new Ack() {
                        @Override
                        public void call(Object... args) {
                            JSONObject data = (JSONObject) args[0];
                            String message = data.toString();
                            Log.d("RoomAutomation", message);
                        }
                    });

                }
                */
            }
        });

        /*
        // Spinner element
        comp5Spinner = (Spinner) findViewById(R.id.comp5_spinner);

        // Spinner Drop down elements
        List<Integer> categories = new ArrayList<Integer>();

        // loop that goes through list
        for (int j=0;j<6;j++) {
            categories.add(j);
        }

        // Creating adapter for spinner
        ArrayAdapter<Integer> dataAdapter = new ArrayAdapter<Integer>(getApplicationContext(), android.R.layout.simple_spinner_item, categories);

        // Drop down layout style - list view with radio button
        dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

        // attaching data adapter to spinner
        comp5Spinner.setAdapter(dataAdapter);
        comp5Spinner.setSelection(dbHelper.getModuleValue(clientID, "Comp5"));

        // Spinner click listener
        comp5Spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                int speed = (int) parent.getItemAtPosition(position);
                if (local == 1) {
                    parameterValue = clientID.substring(4) + "  Comp5  " + String.valueOf(speed);
                    // execute HTTP request
                    if (ipAddress.length() > 0 && portNumber.length() > 0) {
                        new HttpRequestAsyncTask(
                                getApplicationContext(), parameterValue, ipAddress, portNumber, "cmd"
                        ).execute();
                    }

                } else {
                    //scket-io code
                    try {
                        // Sending an object
                        obj = new JSONObject();
                        obj.put("devID", clientID.substring(4));
                        obj.put("comp", "Comp5");
                        obj.put("val",speed);
                    }catch (JSONException e) {
                        Log.d("RoomAutomation", e.toString());
                    }
                    mSocket.emit("command", obj, new Ack() {
                        @Override
                        public void call(Object... args) {
                            JSONObject data = (JSONObject) args[0];
                            String message = data.toString();
                            Log.d("RoomAutomation", message);
                        }
                    });

                }

            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });
        */
        mSocket.on("response",onResponse);
        //added by viru on 29-4-2017
        mSocket.on("disconnect",onDisconnection);
        //added by viru on 30-4-2017
        mSocket.on("reconnect",onReconnection);

        // get the IP address and port number from the last time the user used the app,
        // put an empty string "" is this is the first time.
        //editTextIPAddress.setText(sharedPreferences.getString(PREF_IP,""));
        //editTextPortNumber.setText(sharedPreferences.getString(PREF_PORT,""));

        /*
        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });
        */
    }


    private Emitter.Listener onResponse = new Emitter.Listener() {
        @Override
        public void call(final Object... args) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    JSONObject data = (JSONObject) args[0];
                    String devid;
                    String comp;
                    int mod,stat,val;
                    try {
                        devid = "RSW-"+data.getString("devID");
                        comp = data.getString("comp");
                        Log.d("TestStub", comp+" ON/OFF response received");
                        mod = data.getInt("mod");
                        stat = data.getInt("stat");
                        val = data.getInt("val");
                        if (comp.indexOf("Comp0") != -1) {
                            if (stat == 1)
                                buttonComp0.setText("ON");
                            else
                                buttonComp0.setText("OFF");
                            comp0SeekBar.setProgress((1000 - val) / 200);
                            if (mod == 1) {
                                comp0Switch.setEnabled(true);
                                comp0Switch.setText("A");
                            } else {
                                comp0Switch.setEnabled(false);
                                comp0Switch.setText("M");
                            }

                        }
                        else if (comp.indexOf("Comp1") != -1) {
                            if (stat == 1)
                                buttonComp1.setText("ON");
                            else
                                buttonComp1.setText("OFF");
                            comp1SeekBar.setProgress((1000 - val) / 200);
                            if (mod == 1) {
                                comp1Switch.setEnabled(true);
                                comp1Switch.setText("A");
                            } else {
                                comp1Switch.setEnabled(false);
                                comp1Switch.setText("M");
                            }

                        }
                        else if (comp.indexOf("Comp2") != -1) {
                            if (stat == 1)
                                buttonComp2.setText("ON");
                            else
                                buttonComp2.setText("OFF");
                            if (mod == 1) {
                                comp2Switch.setEnabled(true);
                                comp2Switch.setText("A");
                            } else {
                                comp2Switch.setEnabled(false);
                                comp2Switch.setText("M");
                            }

                        }
                        dbHelper.updateModule(devid, comp, mod, stat, val);
                    } catch (JSONException e) {
                        Log.d("RoomAutomation", e.toString());
                    }

                }
            });
        }
    };


    //added by viru on 29-4-2017
    private Emitter.Listener onDisconnection = new Emitter.Listener() {
        @Override
        public void call(final Object... args) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
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

                }
            });
        }
    };


    //added by viru on 30-4-2017
    private Emitter.Listener onReconnection = new Emitter.Listener() {
        @Override
        public void call(final Object... args) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {

                    mSocket.once(Socket.EVENT_RECONNECT, new Emitter.Listener() {
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

                }
            });
        }
    };


    @Override
    protected void onDestroy() {
        super.onDestroy();
        mSocket.disconnect();
        mSocket.close();
    }

    private BroadcastReceiver mStatusUpdateMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            // Get extra data included in the Intent
            String status = intent.getStringExtra("status");
            //String clientid = intent.getStringExtra("clientid");
            Log.d("RoomAutomation", "Got message: " + status);


            String[] strtok = status.split(";");
            if(strtok.length > 0) {
                String client = "RSW-" + strtok[0];
                String comp = strtok[1];
                int mod = Integer.parseInt(strtok[2]);
                int stat = Integer.parseInt(strtok[3]);
                //int val = Integer.parseInt(strtok[4]);
                int val = (Integer.parseInt(strtok[4]) > 1000) ? 1000 : Integer.parseInt(strtok[4]) ;
                Log.d("RoomAutomation", "SwitchActivity mStatusUpdateMessageReceiver client : " + client);
                Log.d("RoomAutomation", "SwitchActivity mStatusUpdateMessageReceiver comp : " + comp);
                Log.d("RoomAutomation", "SwitchActivity mStatusUpdateMessageReceiver mod : " + mod);
                Log.d("RoomAutomation", "SwitchActivity mStatusUpdateMessageReceiver stat : " + stat);
                Log.d("RoomAutomation", "SwitchActivity mStatusUpdateMessageReceiver val : " + val);

                if (comp.indexOf("Comp0") != -1) {
                    if (stat == 1)
                        buttonComp0.setText("ON");
                    else
                        buttonComp0.setText("OFF");
                    if (!userChanged)
                        comp0SeekBar.setProgress((1000 - val)/200);
                    else
                        userChanged = false;
                    if (mod == 1) {
                        comp0Switch.setEnabled(true);
                        comp0Switch.setText("A");
                    } else {
                        comp0Switch.setEnabled(false);
                        comp0Switch.setText("M");
                    }
                } else if (comp.indexOf("Comp1") != -1) {
                    if (stat == 1)
                        buttonComp1.setText("ON");
                    else
                        buttonComp1.setText("OFF");
                    if (!userChanged)
                        comp1SeekBar.setProgress((1000 - val)/200);
                    else
                        userChanged = false;
                    if (mod == 1) {
                        comp1Switch.setEnabled(true);
                        comp1Switch.setText("A");
                    } else {
                        comp1Switch.setEnabled(false);
                        comp1Switch.setText("M");
                    }

                } else if (comp.indexOf("Comp2") != -1) {
                    if (stat == 1)
                        buttonComp2.setText("ON");
                    else
                        buttonComp2.setText("OFF");
                    if (mod == 1) {
                        comp2Switch.setEnabled(true);
                        comp2Switch.setText("A");
                    } else {
                        comp2Switch.setEnabled(false);
                        comp2Switch.setText("M");
                    }

                }
                //dbHelper.updateModule(client, comp, mod, stat, val);
                //Log.d("RoomAutomation", "mStatusUpdateMessageReceiver Comp 2 status: " + dbHelper.getModuleStatus(client, "Comp2"));
            }

        }
    };

    @Override
    protected void onResume() {
        // Register to receive message.
        LocalBroadcastManager.getInstance(this).registerReceiver(
                mStatusUpdateMessageReceiver, new IntentFilter("status-update"));
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
        // Unregister since the activity is paused.
        LocalBroadcastManager.getInstance(this).unregisterReceiver(
                mStatusUpdateMessageReceiver);

    }

    @Override
    public void onClick(View view) {

        /*
        // get the pin number
        String parameterValue = "";
        // get the ip address
        String ipAddress = "192.168.4.1";
        // get the port number
        String portNumber = "5000";
        String pName = "";
        */
        // save the IP address and port for the next time the app is used
        //editor.putString(PREF_IP, ipAddress); // set the ip address value to save
        //editor.putString(PREF_PORT, portNumber); // set the port number to save
        //editor.commit(); // save the IP and PORT

        // get the pin number from the button that was clicked
        if(view.getId()==buttonComp0.getId())
        {
            Log.d("RoomAutomation","Comp0 of/off selected");
            if (local == 1) {
                if(dbHelper.getModuleStatus(clientID,"Comp0") == 1)
                    parameterValue = clientID.substring(4)+";Comp0;"+String.valueOf(dbHelper.getModuleMode(clientID, "Comp0"))+";0;"+String.valueOf(dbHelper.getModuleValue(clientID,"Comp0"))+";";
                else
                    parameterValue = clientID.substring(4)+";Comp0;"+String.valueOf(dbHelper.getModuleMode(clientID, "Comp0"))+";1;"+String.valueOf(dbHelper.getModuleValue(clientID,"Comp0"))+";";

                // execute HTTP request
                if(ipAddress.length()>0 && portNumber.length()>0) {
                    new HttpRequestAsyncTask(
                            getApplicationContext(), parameterValue, ipAddress, portNumber, "cmd"
                    ).execute();
                }

            }
            else {
                //socket-io code
                int state;
                if(dbHelper.getModuleStatus(clientID,"Comp0") == 1)
                    state = 0;
                else
                    state = 1;

                try {
                    // Sending an object
                    obj = new JSONObject();
                    obj.put("serverID", serverID.substring(4));
                    obj.put("devID", clientID.substring(4));
                    obj.put("comp", "Comp0");
                    obj.put("mod", dbHelper.getModuleMode(clientID, "Comp0"));
                    obj.put("stat",state);
                    obj.put("val", dbHelper.getModuleValue(clientID, "Comp0"));
                }catch (JSONException e) {
                    Log.d("RoomAutomation", e.toString());
                }
                mSocket.emit("command", obj, new Ack() {
                    @Override
                    public void call(Object... args) {
                        JSONObject data = (JSONObject) args[0];
                        String message = data.toString();
                        Log.d("RoomAutomation", message);
                    }
                });

            }
        }
        else if(view.getId()==buttonComp1.getId())
        {
            Log.d("RoomAutomation","Comp1 of/off selected");
            if (local == 1) {
                if(dbHelper.getModuleStatus(clientID,"Comp1") == 1)
                    parameterValue = clientID.substring(4)+";Comp1;"+String.valueOf(dbHelper.getModuleMode(clientID, "Comp1"))+";0;"+String.valueOf(dbHelper.getModuleValue(clientID,"Comp1"))+";";
                else
                    parameterValue = clientID.substring(4)+";Comp1;"+String.valueOf(dbHelper.getModuleMode(clientID, "Comp1"))+";1;"+String.valueOf(dbHelper.getModuleValue(clientID,"Comp1"))+";";

                // execute HTTP request
                if(ipAddress.length()>0 && portNumber.length()>0) {
                    new HttpRequestAsyncTask(
                            getApplicationContext(), parameterValue, ipAddress, portNumber, "cmd"
                    ).execute();
                }

            }
            else {
                //socket-io code
                int state;
                if(dbHelper.getModuleStatus(clientID,"Comp1") == 1)
                    state = 0;
                else
                    state = 1;

                try {
                    // Sending an object
                    obj = new JSONObject();
                    obj.put("serverID", serverID.substring(4));
                    obj.put("devID", clientID.substring(4));
                    obj.put("comp", "Comp1");
                    obj.put("mod", dbHelper.getModuleMode(clientID, "Comp1"));
                    obj.put("stat",state);
                    obj.put("val", dbHelper.getModuleValue(clientID, "Comp1"));
                }catch (JSONException e) {
                    Log.d("RoomAutomation", e.toString());
                }
                mSocket.emit("command", obj, new Ack() {
                    @Override
                    public void call(Object... args) {
                        JSONObject data = (JSONObject) args[0];
                        String message = data.toString();
                        Log.d("RoomAutomation", message);
                    }
                });

            }
        }
        else if(view.getId()==buttonComp2.getId())
        {
            if (local == 1) {
                Log.d("RoomAutomation","Comp 2 status: "+dbHelper.getModuleStatus(clientID,"Comp2"));
                if(dbHelper.getModuleStatus(clientID,"Comp2") == 1)
                    parameterValue = clientID.substring(4)+";Comp2;"+String.valueOf(dbHelper.getModuleMode(clientID, "Comp2"))+";0;"+String.valueOf(dbHelper.getModuleValue(clientID,"Comp2"))+";";
                else
                    parameterValue = clientID.substring(4)+";Comp2;"+String.valueOf(dbHelper.getModuleMode(clientID, "Comp2"))+";1;"+String.valueOf(dbHelper.getModuleValue(clientID,"Comp2"))+";";

                // execute HTTP request
                if(ipAddress.length()>0 && portNumber.length()>0) {
                    new HttpRequestAsyncTask(
                            getApplicationContext(), parameterValue, ipAddress, portNumber, "cmd"
                    ).execute();
                }

            }
            else {
                //socket-io code
                int state;
                if(dbHelper.getModuleStatus(clientID,"Comp2") == 1)
                    state = 0;
                else
                    state = 1;

                try {
                    // Sending an object
                    obj = new JSONObject();
                    obj.put("serverID", serverID.substring(4));
                    obj.put("devID", clientID.substring(4));
                    obj.put("comp", "Comp2");
                    obj.put("mod", dbHelper.getModuleMode(clientID, "Comp2"));
                    obj.put("stat",state);
                    obj.put("val", dbHelper.getModuleValue(clientID, "Comp2"));
                }catch (JSONException e) {
                    Log.d("RoomAutomation", e.toString());
                }
                mSocket.emit("command", obj, new Ack() {
                    @Override
                    public void call(Object... args) {
                        JSONObject data = (JSONObject) args[0];
                        String message = data.toString();
                        Log.d("RoomAutomation", message);
                    }
                });

            }
        }
        else if(view.getId()==buttonTestStart.getId())
        {
            //startTest = 1;
            //TestStub();
            testStub.start();
            //testThread = new Thread(testStub);
            //testThread.start();

        }
        else if(view.getId()==buttonTestStop.getId())
        {
            //testThread.interrupt();
            testStub.stop();
            //startTest = 0;
        }

        /*
        // execute HTTP request
        if(ipAddress.length()>0 && portNumber.length()>0) {
            new HttpRequestAsyncTask(
                    view.getContext(), parameterValue, ipAddress, portNumber, pName
            ).execute();
        }
        */
    }

    class TestStub implements Runnable {
        Thread backgroundThread;

        public void start() {
            if( backgroundThread == null ) {
                try {
                    backgroundThread = new Thread(this);
                    backgroundThread.start();
                }catch (Exception e) {
                    Log.d("RoomAutomation","Thread start exception: "+e.toString());
                }
            }
        }

        public void stop() {
            if( backgroundThread != null ) {
                backgroundThread.interrupt();
            }
        }

        @Override
        public void run() {


            //private void TestStub() {
            String comp;
            //while (startTest == 1) {
            try {
                while (!backgroundThread.interrupted()) {
                    for (int i = 0; i < 3; i++) {
                        comp = "Comp" + String.valueOf(i);
                        //Log.d("TestStub", comp+" command triggered");
                        if (local == 1) {
                            if (dbHelper.getModuleStatus(clientID, comp) == 1)
                                parameterValue = clientID.substring(4) + ";" + comp + ";" + String.valueOf(dbHelper.getModuleMode(clientID, comp)) + ";0;" + String.valueOf(dbHelper.getModuleValue(clientID, comp)) + ";";
                            else
                                parameterValue = clientID.substring(4) + ";" + comp + ";" + String.valueOf(dbHelper.getModuleMode(clientID, comp)) + ";1;" + String.valueOf(dbHelper.getModuleValue(clientID, comp)) + ";";

                            // execute HTTP request
                            if (ipAddress.length() > 0 && portNumber.length() > 0) {
                                new HttpRequestAsyncTask(
                                        getApplicationContext(), parameterValue, ipAddress, portNumber, "cmd"
                                ).execute();
                            }

                        } else {
                            //socket-io code
                            int state;
                            if (dbHelper.getModuleStatus(clientID, comp) == 1)
                                state = 0;
                            else
                                state = 1;

                            try {
                                // Sending an object
                                obj = new JSONObject();
                                obj.put("serverID", serverID.substring(4));
                                obj.put("devID", clientID.substring(4));
                                obj.put("comp", comp);
                                obj.put("mod", dbHelper.getModuleMode(clientID, comp));
                                obj.put("stat", state);
                                obj.put("val", dbHelper.getModuleValue(clientID, comp));
                            } catch (JSONException e) {
                                Log.d("RoomAutomation", e.toString());
                            }
                            Log.d("TestStub", comp + " ON/OFF command triggered");
                            mSocket.emit("command", obj, new Ack() {
                                @Override
                                public void call(Object... args) {
                                    //JSONObject data = (JSONObject) args[0];
                                    //String message = data.toString();
                                    //Log.d("RoomAutomation", message);
                                    Log.d("RoomAutomation", args[0].toString());
                                }
                            });

                        }
                        backgroundThread.sleep(2000);
                    }
                    //try {
                        //backgroundThread.sleep(3000);
                    //} catch (InterruptedException e) {
                        //e.printStackTrace();
                    //}
                    //SystemClock.sleep(1000);
                }
            }catch( InterruptedException ex ) {
                // important you respond to the InterruptedException and stop processing
                // when its thrown!  Notice this is outside the while loop.
                //Log.d("Thread shutting down as it was requested to stop.");
            } finally {
                backgroundThread = null;
            }
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_room, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }


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

        /**
         * Description: The asyncTask class constructor. Assigns the values used in its other methods.
         * @param context the application context, needed to create the dialog
         * @param parameterValue the pin number to toggle
         * @param ipAddress the ip address to send the request to
         * @param portNumber the port number of the ip address
         */
        public HttpRequestAsyncTask(Context context, String parameterValue, String ipAddress, String portNumber, String parameter)
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
            //alertDialog.setMessage(requestReply);
            Log.d("RoomAutomation", "SwitchActivity onPostExecute"+requestReply);
            userChanged = true;
            /*
            String[] strtok = requestReply.split(";");
            if(strtok.length > 0) {
                String client = "RSW-" + strtok[0];
                String comp = strtok[1];
                int mod = Integer.parseInt(strtok[2]);
                int stat = Integer.parseInt(strtok[3]);
                int val = Integer.parseInt(strtok[4]);
                Log.d("RoomAutomation", "SwitchActivity onPostExecute client : " + client);
                Log.d("RoomAutomation", "SwitchActivity onPostExecute comp : " + comp);
                Log.d("RoomAutomation", "SwitchActivity onPostExecute mod : " + mod);
                Log.d("RoomAutomation", "SwitchActivity onPostExecute stat : " + stat);
                Log.d("RoomAutomation", "SwitchActivity onPostExecute val : " + val);

                if (comp.indexOf("Comp0") != -1) {
                    if (stat == 1)
                        buttonComp0.setText("ON");
                    else
                        buttonComp0.setText("OFF");
                    //comp0SeekBar.setProgress(1000 - val);
                    if (mod == 1) {
                        comp0Switch.setEnabled(true);
                        comp0Switch.setText("A");
                    } else {
                        comp0Switch.setEnabled(false);
                        comp0Switch.setText("M");
                    }
                } else if (comp.indexOf("Comp1") != -1) {
                    if (stat == 1)
                        buttonComp1.setText("ON");
                    else
                        buttonComp1.setText("OFF");
                    //comp1SeekBar.setProgress(1000 - val);
                    if (mod == 1) {
                        comp1Switch.setEnabled(true);
                        comp1Switch.setText("A");
                    } else {
                        comp1Switch.setEnabled(false);
                        comp1Switch.setText("M");
                    }

                } else if (comp.indexOf("Comp2") != -1) {
                    if (stat == 1)
                        buttonComp2.setText("ON");
                    else
                        buttonComp2.setText("OFF");
                    if (mod == 1) {
                        comp2Switch.setEnabled(true);
                        comp2Switch.setText("A");
                    } else {
                        comp2Switch.setEnabled(false);
                        comp2Switch.setText("M");
                    }

                }
                dbHelper.updateModule(client, comp, mod, stat, val);
                Log.d("RoomAutomation", "postExecute Comp 2 status: " + dbHelper.getModuleStatus(client, "Comp2"));
            }
            */
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
        }

    }


}
