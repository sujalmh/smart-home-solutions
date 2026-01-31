package com.v2innovations.roomautomation;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.graphics.Color;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.Toast;

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

/**
 * Created by virupaksha on 18-11-2016.
 */

public class ConfigActivity extends AppCompatActivity implements View.OnClickListener {
    public final static String PREF_IP = "PREF_IP_ADDRESS";
    public final static String PREF_PORT = "PREF_PORT_NUMBER";
    // declare buttons and text inputs
    private Button buttonConfig;
    private EditText editTextSSID, editTextPWD;
    String devID = "";
    RadioGroup radioDevType;
    LinearLayout mainLayout;
    RoomAutomationDBHelper dbHelper = null;
    String ipAddress = "192.168.4.100";
    //String ipAddress = "192.168.4.1";
    String portNumber = "6000";
    String parameterValue = "";
    String serverID = "";

    // shared preferences objects used to save the IP address and port so that the user doesn't have to
    // type them next time he/she opens the app.
    SharedPreferences.Editor editor;
    SharedPreferences sharedPreferences;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.dev_setas);
        //Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        //setSupportActionBar(toolbar);


        sharedPreferences = getSharedPreferences("HTTP_HELPER_PREFS", Context.MODE_PRIVATE);
        editor = sharedPreferences.edit();

        mainLayout = (LinearLayout)findViewById(R.id.setasLayout);
        Intent i = getIntent();
        devID = (String ) i.getStringExtra("selecteddevice");

        //dbHelper = new RoomAutomationDBHelper(getApplicationContext());
        final RoomAutomationApp app = (RoomAutomationApp) getApplication();
        dbHelper = app.getDbHelper();

        radioDevType = (RadioGroup) findViewById(R.id.devtype_rg);

        radioDevType.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                //RadioButton checkedRadioButton = (RadioButton)group.findViewById(checkedId);
                switch (checkedId) {
                    case R.id.server_rb:
                        dbHelper.insertServer(devID, devID.substring(4));
                        parameterValue = "";
                        if (ipAddress.length() > 0 && portNumber.length() > 0) {
                            new HttpRequestAsyncTask(
                                    getApplicationContext(), parameterValue, ipAddress, portNumber, "ser"
                            ).execute();
                        }

                        View view = LayoutInflater.from(getApplicationContext()).inflate(R.layout.remote_config, null);
                        mainLayout.addView(view);
                        editTextSSID = (EditText) findViewById(R.id.ssid_et);
                        editTextSSID.setTextColor(Color.BLACK);
                        editTextPWD = (EditText) findViewById(R.id.pw_et);
                        editTextPWD.setTextColor(Color.BLACK);
                        buttonConfig = (Button) findViewById(R.id.config_btn);
                        buttonConfig.setOnClickListener(new View.OnClickListener() {
                            @Override
                            public void onClick(View v) {
                                String ssid = editTextSSID.getText().toString().trim();
                                // get the port number
                                String pwd = editTextPWD.getText().toString().trim();
                                if (ssid == "") {
                                    Toast.makeText(getApplicationContext(), "Please enter WiFi SSID", Toast.LENGTH_LONG).show();
                                } else if (pwd == "") {
                                    Toast.makeText(getApplicationContext(), "Please enter Password", Toast.LENGTH_LONG).show();
                                } else {
                                    parameterValue = ssid + ";" + pwd + ";";
                                    // execute HTTP request
                                    ipAddress = dbHelper.getServerIP(devID);
                                    if (ipAddress.length() > 0 && portNumber.length() > 0) {
                                        new HttpRequestAsyncTask(
                                                v.getContext(), parameterValue, ipAddress, portNumber, "rem"
                                        ).execute();
                                    }

                                }
                            }
                        });
                        break;

                    case R.id.client_rb:
                        View cview = LayoutInflater.from(getApplicationContext()).inflate(R.layout.client_config, null);
                        mainLayout.addView(cview);

                        // Spinner element
                        Spinner spinner = (Spinner) findViewById(R.id.clidev_spinner);

                        // Spinner Drop down elements
                        List<String> categories = new ArrayList<String>();

                        Cursor cursor = dbHelper.getAllServers();

                        if (cursor.moveToFirst()) {
                            do {
                                categories.add(cursor.getString(0));
                            } while (cursor.moveToNext());
                        }


                        // Creating adapter for spinner
                        ArrayAdapter<String> dataAdapter = new ArrayAdapter<String>(getApplicationContext(), android.R.layout.simple_spinner_item, categories);

                        // Drop down layout style - list view with radio button
                        dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

                        // attaching data adapter to spinner
                        spinner.setAdapter(dataAdapter);
                        spinner.setBackgroundColor(Color.BLACK);

                        // Spinner click listener
                        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                            @Override
                            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                                serverID = parent.getItemAtPosition(position).toString();
                            }

                            @Override
                            public void onNothingSelected(AdapterView<?> parent) {

                            }
                        });

                        //editTextSSID = (EditText)findViewById(R.id.clissid_et);
                        editTextPWD = (EditText) findViewById(R.id.clipw_et);
                        editTextPWD.setTextColor(Color.BLACK);
                        editTextPWD.setVisibility(View.INVISIBLE);
                        //editTextPWD.setText(serverID.substring(4));

                        buttonConfig = (Button) findViewById(R.id.cliconfig_btn);
                        buttonConfig.setOnClickListener(new View.OnClickListener() {
                            @Override
                            public void onClick(View v) {
                                String ssid = serverID;
                                // get the port number
                                String pwd = serverID.substring(4);//editTextPWD.getText().toString().trim();
                                if (ssid == "") {
                                    Toast.makeText(getApplicationContext(), "Please select the Server SSID", Toast.LENGTH_LONG).show();
                                } else if (pwd == "") {
                                    Toast.makeText(getApplicationContext(), "Please enter Password", Toast.LENGTH_LONG).show();
                                } else {
                                    int cInd = app.getClientInd();
                                    String cIP = "192.168.4."+String.valueOf(cInd);
                                    dbHelper.insertClient(serverID, devID, pwd,cIP);
                                    parameterValue = ssid + ";" + pwd + ";"+cIP;
                                    ipAddress = "192.168.4.1";
                                    // execute HTTP request
                                    if (ipAddress.length() > 0 && portNumber.length() > 0) {
                                        new HttpRequestAsyncTask(
                                                v.getContext(), parameterValue, ipAddress, portNumber, "cli"
                                        ).execute();
                                    }

                                }
                            }
                        });
                        break;
                    case R.id.both_rb:
                        dbHelper.insertServer(devID, devID.substring(4));
                        int cInd = app.getClientInd();
                        String cIP = "192.168.4."+String.valueOf(cInd);

                        dbHelper.insertClient(devID, devID, devID.substring(4),cIP);
                        parameterValue = "";
                        if (ipAddress.length() > 0 && portNumber.length() > 0) {
                            new HttpRequestAsyncTask(
                                    getApplicationContext(), parameterValue, ipAddress, portNumber, "ser"
                            ).execute();
                        }

                        View view1 = LayoutInflater.from(getApplicationContext()).inflate(R.layout.remote_config, null);
                        mainLayout.addView(view1);
                        editTextSSID = (EditText) findViewById(R.id.ssid_et);
                        editTextSSID.setTextColor(Color.BLACK);
                        editTextPWD = (EditText) findViewById(R.id.pw_et);
                        editTextPWD.setTextColor(Color.BLACK);
                        buttonConfig = (Button) findViewById(R.id.config_btn);
                        buttonConfig.setOnClickListener(new View.OnClickListener() {
                            @Override
                            public void onClick(View v) {
                                String ssid = editTextSSID.getText().toString().trim();
                                // get the port number
                                String pwd = editTextPWD.getText().toString().trim();
                                if (ssid == "") {
                                    Toast.makeText(getApplicationContext(), "Please enter WiFi SSID", Toast.LENGTH_LONG).show();
                                } else if (pwd == "") {
                                    Toast.makeText(getApplicationContext(), "Please enter Password", Toast.LENGTH_LONG).show();
                                } else {
                                    parameterValue = ssid + ";" + pwd + ";";
                                    // execute HTTP request
                                    ipAddress = dbHelper.getServerIP(devID);
                                    if (ipAddress.length() > 0 && portNumber.length() > 0) {
                                        new HttpRequestAsyncTask(
                                                v.getContext(), parameterValue, ipAddress, portNumber, "rem"
                                        ).execute();
                                    }

                                }
                            }
                        });
                    break;

                }
            }

        });

        /*
        // assign buttons
        buttonConfig = (Button)findViewById(R.id.config_btn);

        // assign text inputs
        editTextSSID = (EditText)findViewById(R.id.ssid_et);
        editTextPWD = (EditText)findViewById(R.id.pw_et);

        // set button listener (this class)
        buttonConfig.setOnClickListener(this);
        */

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

        String ipAddress = "192.168.4.1";
        String portNumber = "6000";
        // get the pin number
        String parameterValue = "";
        // get the ip address
        String ssid = editTextSSID.getText().toString().trim();
        // get the port number
        String pwd = editTextPWD.getText().toString().trim();



        // get the pin number from the button that was clicked
        if(view.getId()==buttonConfig.getId())
        {
            parameterValue = ssid+";"+pwd+";";
        }



        // execute HTTP request
        if(ipAddress.length()>0 && portNumber.length()>0) {
            new HttpRequestAsyncTask(
                    view.getContext(), parameterValue, ipAddress, portNumber, "con"
            ).execute();
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
            URI website = new URI("http://"+ipAddress+":"+portNumber+"/?"+parameterName+"="+parameterValue);
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

            alertDialog = new AlertDialog.Builder(this.context)
                    .setTitle("HTTP Response From IP Address:")
                    .setCancelable(true)
                    .create();

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
            /*
            alertDialog.setMessage(requestReply);
            if(!alertDialog.isShowing())
            {
                alertDialog.show(); // show dialog
            }
            */
            Log.d("RoomAutomation-", "onPostExecute: "+requestReply);
            if (requestReply.contains("Remote Configured")){
                finish();
            }
            else if (requestReply.contains("Server Configured")){
                /*
                dbHelper.updateServerIP(devID,"192.168.4.100");
                WifiConfiguration wifiConfig = new WifiConfiguration();
                wifiConfig.SSID = String.format("\"%s\"", devID);
                String pword = devID.substring(4);
                wifiConfig.preSharedKey = String.format("\"%s\"", pword);

                WifiManager wifiManager = (WifiManager)getSystemService(WIFI_SERVICE);
                //remember id
                int netId = wifiManager.addNetwork(wifiConfig);
                wifiManager.disconnect();
                wifiManager.enableNetwork(netId, true);
                wifiManager.reconnect();
                */
            }
            else if (requestReply.contains("Client Configured")){
                finish();
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
        }

    }


}
