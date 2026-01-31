package com.v2innovations.roomautomation;

import android.Manifest;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.DhcpInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.PopupMenu;
import android.support.v7.widget.Toolbar;
import android.text.format.Formatter;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by virupaksha on 18-11-2016.
 */

public class SelectActivity extends AppCompatActivity implements View.OnClickListener {
    public final static String PREF_IP = "PREF_IP_ADDRESS";
    public final static String PREF_PORT = "PREF_PORT_NUMBER";
    private static final int PERMISSION_REQUEST_COARSE_LOCATION = 1;
    // declare buttons and text inputs
    private Button buttonConnect;
    private Spinner devSpinner;
    RoomAutomationDBHelper dbHelper = null;
    Cursor cursor;
    String devID = "";
    List<String> categories = new ArrayList<String>();
    // shared preferences objects used to save the IP address and port so that the user doesn't have to
    // type them next time he/she opens the app.
    SharedPreferences.Editor editor;
    SharedPreferences sharedPreferences;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.select_dev);
        //Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        //setSupportActionBar(toolbar);

        sharedPreferences = getSharedPreferences("HTTP_HELPER_PREFS", Context.MODE_PRIVATE);
        editor = sharedPreferences.edit();

        // assign buttons
        buttonConnect = (Button)findViewById(R.id.cconnect_btn);

        // set button listener (this class)
        buttonConnect.setOnClickListener(this);

        // Spinner element
        devSpinner = (Spinner) findViewById(R.id.cdev_spinner);

        // Spinner Drop down elements
        //List<String> categories = new ArrayList<String>();

        WifiManager wifi= (WifiManager) getSystemService(Context.WIFI_SERVICE);

        //wifi.startScan();
        /*
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (this.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                final AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle("This App needs location access");
                builder.setMessage("Please grant location access so this app can detect WiFi's.");
                builder.setPositiveButton(android.R.string.ok, null);
                builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                            requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, PERMISSION_REQUEST_COARSE_LOCATION);
                        }
                    }
                });
            }
        }
        */
        /*
        if (this.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) { 
            final AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setTitle("This app needs location access");
            builder.setMessage("Please grant location access so this app can detect WiFi's.");
            builder.setPositiveButton(android.R.string.ok, null);
            builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                @Override
                public void onDismiss(DialogInterface dialog) {
                    requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, PERMISSION_REQUEST_COARSE_LOCATION); 
                }
            });
            builder.show();
        }
        */

        //ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.ACCESS_COARSE_LOCATION},
          //      PERMISSION_REQUEST_COARSE_LOCATION);

        // get list of the results in object format ( like an array )
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION)
        == PackageManager.PERMISSION_GRANTED) {

            wifi.startScan();
            List<ScanResult> results = wifi.getScanResults();

            // loop that goes through list
            for (ScanResult result : results) {
                //Toast.makeText(this, result.SSID + " " + result.level,
                //Toast.LENGTH_SHORT).show();
                categories.add(result.SSID);
            }

            if (!categories.isEmpty()) {
                // Creating adapter for spinner
                ArrayAdapter<String> dataAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, categories);

                // Drop down layout style - list view with radio button
                dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

                // attaching data adapter to spinner
                devSpinner.setAdapter(dataAdapter);

                // Spinner click listener
                devSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                    @Override
                    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                        devID = parent.getItemAtPosition(position).toString();
                    }

                    @Override
                    public void onNothingSelected(AdapterView<?> parent) {

                    }
                });
            }
        }
        else {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.ACCESS_COARSE_LOCATION},
                    PERMISSION_REQUEST_COARSE_LOCATION);
        }
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
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        WifiManager wifi= (WifiManager) getSystemService(Context.WIFI_SERVICE);
        categories.clear();

        switch (requestCode) {
            case PERMISSION_REQUEST_COARSE_LOCATION: {
                if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Log.d("RoomAutomation", "coarse location permission granted");

                    wifi.startScan();
                    List<ScanResult> results = wifi.getScanResults();

                    // loop that goes through list
                    for (ScanResult result : results) {
                        //Toast.makeText(this, result.SSID + " " + result.level,
                        //Toast.LENGTH_SHORT).show();
                        categories.add(result.SSID);
                    }
                    /*
                    synchronized(categories){
                        categories.notifyAll();
                    }
                    */
                    if (!categories.isEmpty()) {
                        // Creating adapter for spinner
                        ArrayAdapter<String> dataAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, categories);

                        // Drop down layout style - list view with radio button
                        dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

                        // attaching data adapter to spinner
                        devSpinner.setAdapter(dataAdapter);

                        // Spinner click listener
                        devSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                            @Override
                            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                                devID = parent.getItemAtPosition(position).toString();
                            }

                            @Override
                            public void onNothingSelected(AdapterView<?> parent) {

                            }
                        });
                    }
                    //categories.notifyAll();

                } else {
                    final AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    builder.setTitle("Functionality limited");
                    builder.setMessage("Since location access has not been granted, this app will not be able to discover WiFi when in the background.");
                    builder.setPositiveButton(android.R.string.ok, null);
                    builder.setOnDismissListener(new DialogInterface.OnDismissListener() {

                        @Override
                        public void onDismiss(DialogInterface dialog) {
                        }

                    });
                    builder.show();
                }
                return;
            }
        }
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
        if(view.getId()==buttonConnect.getId())
        {
            WifiConfiguration wifiConfig = new WifiConfiguration();
            wifiConfig.SSID = String.format("\"%s\"", devID);
            String pword = devID.substring(4);
            wifiConfig.preSharedKey = String.format("\"%s\"", pword);

            WifiManager wifiManager = (WifiManager)getSystemService(WIFI_SERVICE);
            //remember id
            int netId = wifiManager.addNetwork(wifiConfig);
            wifiManager.disconnect();
            wifiManager.enableNetwork(netId, true);
            //wifiManager.reconnect();

            //WifiInfo wifiInfo = null;
            if (wifiManager.reconnect()) {
                //wifiInfo = wifiManager.getConnectionInfo();
                //int ipAdd = wifiInfo.getIpAddress();
                //Log.d("RoomAutomation", "Dev ipAdd: " + String.valueOf(ipAdd));
                final DhcpInfo dhcp = wifiManager.getDhcpInfo();
                String address = Formatter.formatIpAddress(dhcp.gateway);
                Log.d("RoomAutomation", "Dev ipAdd: " + address);
                //}

                //final DhcpInfo dhcp = wifiManager.getDhcpInfo();
                //String address = Formatter.formatIpAddress(dhcp.gateway);
                //Log.d("RoomAutomation", "Dev ipAdd: " + address);
                Intent i = new Intent(SelectActivity.this, ConfigActivity.class);
                i.putExtra("selecteddevice", devID);
                startActivity(i);
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

}
