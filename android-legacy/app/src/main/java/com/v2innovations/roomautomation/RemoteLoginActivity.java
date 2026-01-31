/**
 * Created by virupaksha on 18-11-2016.
 */

package com.v2innovations.roomautomation;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
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

import io.socket.client.Ack;
import io.socket.client.Socket;
import io.socket.emitter.Emitter;
import org.json.JSONException;
import org.json.JSONObject;

public class RemoteLoginActivity extends AppCompatActivity implements View.OnClickListener {
    public final static String PREF_IP = "PREF_IP_ADDRESS";
    public final static String PREF_PORT = "PREF_PORT_NUMBER";
    // declare buttons and text inputs
    private Button buttonLogin,buttonRegister;
    private EditText editEmail, editPWD;
    private Spinner devSpinner;
    RoomAutomationDBHelper dbHelper = null;
    Cursor cursor;
    String devID = "";
    private Socket mSocket;
    // shared preferences objects used to save the IP address and port so that the user doesn't have to
    // type them next time he/she opens the app.
    SharedPreferences.Editor editor;
    SharedPreferences sharedPreferences;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.remote_login);
        //Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        //setSupportActionBar(toolbar);

        sharedPreferences = getSharedPreferences("HTTP_HELPER_PREFS", Context.MODE_PRIVATE);
        editor = sharedPreferences.edit();

        RoomAutomationApp app = (RoomAutomationApp) getApplication();
        mSocket = app.getSocket();
        mSocket.connect();

        // assign buttons
        buttonLogin = (Button)findViewById(R.id.login_btn);
        buttonRegister = (Button)findViewById(R.id.register_btn);


        // set button listener (this class)
        buttonLogin.setOnClickListener(this);
        buttonRegister.setOnClickListener(this);
        editEmail = (EditText)findViewById(R.id.email_et);
        editPWD = (EditText)findViewById(R.id.pw_et);

        // Spinner element
        Spinner spinner = (Spinner) findViewById(R.id.dev_spinner);

        // Spinner Drop down elements
        List<String> categories = new ArrayList<String>();

        //dbHelper = new RoomAutomationDBHelper(getApplicationContext());
        //RoomAutomationApp app = (RoomAutomationApp) getApplication();
        dbHelper = app.getDbHelper();
        cursor = dbHelper.getAllServers();

        if (cursor.moveToFirst()){
            do {
                categories.add(cursor.getString(0));
            } while (cursor.moveToNext());
        }

        // Creating adapter for spinner
        ArrayAdapter<String> dataAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, categories);

        // Drop down layout style - list view with radio button
        dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

        // attaching data adapter to spinner
        spinner.setAdapter(dataAdapter);

        // Spinner click listener
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                devID = parent.getItemAtPosition(position).toString();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

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
        String EmailID = "";
        String PWD = "";
        JSONObject obj = null;

        // save the IP address and port for the next time the app is used
        editor.putString(PREF_IP, ipAddress); // set the ip address value to save
        editor.putString(PREF_PORT, portNumber); // set the port number to save
        editor.commit(); // save the IP and PORT

        EmailID = editEmail.getText().toString();
        PWD = editPWD.getText().toString();

        // get the pin number from the button that was clicked
        if(view.getId()==buttonLogin.getId())
        {
            if (dbHelper.getEmailID() == ""){
                Toast.makeText(getApplicationContext(), "No User is Registered, please register first", Toast.LENGTH_LONG).show();
            }
            else {
                EmailID = dbHelper.getEmailID();
                editEmail.setText(EmailID);

                if (PWD == "") {
                    Toast.makeText(getApplicationContext(), "Please enter Password", Toast.LENGTH_LONG).show();
                }
                else {
                    try {
                        // S    ending an object
                        obj = new JSONObject();
                        obj.put("name", EmailID);
                        obj.put("pword", PWD);
                        obj.put("devID",devID.substring(4));
                    }catch (JSONException e) {
                        Log.d("RoomAutomation", e.toString());
                    }
                    mSocket.emit("login", obj, new Ack() {
                        @Override
                        public void call(Object... args) {
                            //JSONObject data = (JSONObject) args[0];
                            //String message = data.toString();
                            String message = args[0].toString();
                            if (message.contains("success")) {
                                runOnUiThread(new Runnable() {
                                    public void run() {

                                        Toast.makeText(RemoteLoginActivity.this, "User Successfully Logged In", Toast.LENGTH_LONG).show();
                                    }
                                });

                                //Toast.makeText(RemoteLoginActivity.this, "User Successfully Logged In", Toast.LENGTH_LONG).show();
                                //dbHelper.insertUser(EmailID,PWD,devID);
                                //editEmail.setText("");
                                Intent i = new Intent(RemoteLoginActivity.this, RoomSwitchesActivity.class);
                                i.putExtra("serverid", devID);//dbHelper.getServerID());
                                i.putExtra("local", 0);
                                startActivity(i);
                            }
                            else Log.d("RoomAutomation", message);
                        }
                    });

                }
            }

        }
        else if(view.getId()==buttonRegister.getId())
        {
            if (EmailID == "") {
                Toast.makeText(getApplicationContext(), "Please enter EmailID", Toast.LENGTH_LONG).show();
            }
            else if (PWD == "") {
                Toast.makeText(getApplicationContext(), "Please enter Password", Toast.LENGTH_LONG).show();
            }
            else {
                dbHelper.insertUser(EmailID,PWD,devID);
                try {
                    // Sending an object
                    obj = new JSONObject();
                    obj.put("name", EmailID);
                    obj.put("pword", PWD);
                    obj.put("devID",devID.substring(4));
                }catch (JSONException e) {
                    Log.d("RoomAutomation", e.toString());
                }
                mSocket.emit("new user", obj, new Ack() {
                    @Override
                    public void call(Object... args) {
                        //JSONObject data = (JSONObject) args[0];
                        //String message = data.toString();
                        String message = args[0].toString();
                        if (message.contains("success")) {

                            runOnUiThread(new Runnable() {
                                public void run() {

                                    Toast.makeText(RemoteLoginActivity.this, "User Successfully Registered", Toast.LENGTH_LONG).show();
                                }
                            });

                            //Toast.makeText(RemoteLoginActivity.this, "User Successfully Registered", Toast.LENGTH_LONG).show();
                            //dbHelper.insertUser(EmailID,PWD,devID);
                            //editEmail.setText("");
                            //editPWD.setText("");
                        }
                        else Log.d("RoomAutomation", message);
                    }
                });

            }

        }


    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mSocket.disconnect();
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
