package com.v2innovations.roomautomation;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

public class RoomActivity extends AppCompatActivity implements View.OnClickListener {
    public final static String PREF_IP = "PREF_IP_ADDRESS";
    public final static String PREF_PORT = "PREF_PORT_NUMBER";
    // declare buttons and text inputs
    private Button buttonLocal,buttonRemote,buttonConfig,buttonLocalReg;
    RoomAutomationDBHelper dbHelper = null;
    String serverID = "";
    private EditText editTextIPAddress, editTextPortNumber;
    // shared preferences objects used to save the IP address and port so that the user doesn't have to
    // type them next time he/she opens the app.
    SharedPreferences.Editor editor;
    SharedPreferences sharedPreferences;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_room);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);


        sharedPreferences = getSharedPreferences("HTTP_HELPER_PREFS", Context.MODE_PRIVATE);
        editor = sharedPreferences.edit();

        //dbHelper = new RoomAutomationDBHelper(getApplicationContext());
        RoomAutomationApp app = (RoomAutomationApp) getApplication();
        app.setContext(getApplicationContext());
        dbHelper = app.getDbHelper();
        //dbHelper.deleteAllRecords();
        // assign buttons
        buttonLocal = (Button)findViewById(R.id.lcon_btn);
        buttonRemote = (Button)findViewById(R.id.rlogin_btn);
        buttonConfig = (Button)findViewById(R.id.conf_btn);
        buttonLocalReg = (Button)findViewById(R.id.lreg_btn);


        // set button listener (this class)
        buttonLocal.setOnClickListener(this);
        buttonRemote.setOnClickListener(this);
        buttonConfig.setOnClickListener(this);
        buttonLocalReg.setOnClickListener(this);

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

    @Override
    public void onClick(View view) {

        // get the pin number
        String parameterValue = "";
        // get the ip address
        String ipAddress = "192.168.4.1";//editTextIPAddress.getText().toString().trim();
        // get the port number
        String portNumber = "5000";//editTextPortNumber.getText().toString().trim();


        // save the IP address and port for the next time the app is used
        editor.putString(PREF_IP, ipAddress); // set the ip address value to save
        editor.putString(PREF_PORT, portNumber); // set the port number to save
        editor.commit(); // save the IP and PORT

        // get the pin number from the button that was clicked
        if(view.getId()==buttonLocal.getId())
        {
            if (dbHelper.recordsPresent()) {
                serverID = dbHelper.getServerID();
                WifiConfiguration wifiConfig = new WifiConfiguration();
                wifiConfig.SSID = String.format("\"%s\"", serverID);
                String pword = serverID.substring(4);
                wifiConfig.preSharedKey = String.format("\"%s\"", pword);

                WifiManager wifiManager = (WifiManager)getSystemService(WIFI_SERVICE);
                WifiInfo wifiInfo = wifiManager.getConnectionInfo();
                String ssid = "";
                if ((wifiInfo != null) && (wifiInfo.getSupplicantState() == SupplicantState.COMPLETED)) {
                    ssid = wifiInfo.getSSID();
                }
                if (!ssid.contains(serverID)) {
                    //remember id
                    int netId = wifiManager.addNetwork(wifiConfig);
                    wifiManager.disconnect();
                    wifiManager.enableNetwork(netId, true);
                    if (wifiManager.reconnect()) {

                        //if (dbHelper.clientsPresent()) {
                        Intent i = new Intent(RoomActivity.this, RoomSwitchesActivity.class);
                        i.putExtra("serverid", serverID);
                        i.putExtra("local", 1);
                        startActivity(i);
                    }
                }
                else {
                    Intent i = new Intent(RoomActivity.this, RoomSwitchesActivity.class);
                    i.putExtra("serverid", serverID);
                    i.putExtra("local", 1);
                    startActivity(i);

                }

                    //startActivity(new Intent(RoomActivity.this, RoomSwitchesActivity.class));
                //} else
                    //Toast.makeText(getApplicationContext(), "No clients are configured, configure it using Configure option", Toast.LENGTH_LONG).show();
            }
            else {
                Toast.makeText(getApplicationContext(), "No servers are configured, configure it using Configure option", Toast.LENGTH_LONG).show();
            }
        }
        else if(view.getId()==buttonRemote.getId())
        {
            startActivity(new Intent(RoomActivity.this, RemoteLoginActivity.class));
        }
        else if(view.getId()==buttonConfig.getId())
        {
            startActivity(new Intent(RoomActivity.this, SelectActivity.class));
        }
        else if(view.getId()==buttonLocalReg.getId())
        {
            startActivity(new Intent(RoomActivity.this, RegisterActivity.class));
        }

    }



    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_room, menu);
        return true;
    }

    @Override
    protected void onDestroy() {
        //stopService(new Intent(this, RoomAutomationCurrentStatusService.class));
        super.onDestroy();
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

}
