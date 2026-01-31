package com.v2innovations.roomautomation;

import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by virupaksha on 19-11-2016.
 */

public class RoomSwitchesActivity extends AppCompatActivity implements View.OnClickListener {
    public final static String PREF_IP = "PREF_IP_ADDRESS";
    public final static String PREF_PORT = "PREF_PORT_NUMBER";
    // declare buttons and text inputs
    private Button buttonRSW1,buttonRSW2,buttonRSW3;
    LinearLayout mainLayout;
    private EditText editEmail, editPWD;
    private Spinner devSpinner;
    RoomAutomationDBHelper dbHelper = null;
    Cursor cursor;
    String devID = "";
    int local = 0;
    // shared preferences objects used to save the IP address and port so that the user doesn't have to
    // type them next time he/she opens the app.
    SharedPreferences.Editor editor;
    SharedPreferences sharedPreferences;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.room_switches);
        //Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        //setSupportActionBar(toolbar);

        sharedPreferences = getSharedPreferences("HTTP_HELPER_PREFS", Context.MODE_PRIVATE);
        editor = sharedPreferences.edit();

        Intent i = getIntent();
        local = i.getIntExtra("local",0);
        devID = (String ) i.getStringExtra("serverid");

        mainLayout = (LinearLayout)findViewById(R.id.roomLayout);
        //dbHelper = new RoomAutomationDBHelper(getApplicationContext());
        RoomAutomationApp app = (RoomAutomationApp) getApplication();
        dbHelper = app.getDbHelper();

        Cursor cursor = dbHelper.getAllClients();
        int ind = 1;

        if (cursor.moveToFirst()){
            do {
                /*
                Button button = new Button(this);
                button.setText(cursor.getString(0));
                if (!dbHelper.switchPresent(cursor.getString(0)))
                    dbHelper.insertSwitch(cursor.getString(0));
                // R.id won't be generated for us, so we need to create one
                //noinspection ResourceType
                button.setId(ind);
                LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
                params.gravity = Gravity.CENTER;
                int sizeInDP = 15;

                int marginInDp = (int) TypedValue.applyDimension(
                        TypedValue.COMPLEX_UNIT_DIP, sizeInDP, getResources()
                                .getDisplayMetrics());
                params.topMargin = marginInDp;
                button.setLayoutParams(params);

                // add generated button to view
                mainLayout.addView(button);

                // add our event handler (less memory than an anonymous inner class)
                button.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        Intent intent = new Intent(RoomSwitchesActivity.this, RoomAutomationCurrentStatusService.class);
                        intent.putExtra("clientid",cursor.getString(0));
                        intent.putExtra("local", local);
                        startService(intent);
                    }
                });
                */
                addSwitches(ind,cursor.getString(0));
                ind++;
            } while (cursor.moveToNext());
        }
        else {
            addSwitches(ind,devID);
        }

        //added by viru on 1-5-2017
        if (isMyServiceRunning(RoomAutomationServer.class)) {
            stopService(new Intent(this, RoomAutomationServer.class));
        }

        Intent intent = new Intent(RoomSwitchesActivity.this, RoomAutomationServer.class);
        startService(intent);


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


    //added by viru on 1-5-2017
    private boolean isMyServiceRunning(Class<?> serviceClass) {
        ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (serviceClass.getName().equals(service.service.getClassName())) {
                return true;
            }
        }
        return false;
    }


    private void addSwitches(int ind, String clientID) {
        Button button = new Button(this);
        button.setText(clientID);//cursor.getString(0));
        final String client = clientID;
        Log.d("RoomAutomation","addSwitches client: "+client);
        if (!dbHelper.switchPresent(client))
            dbHelper.insertSwitch(client);
        // R.id won't be generated for us, so we need to create one
        //noinspection ResourceType
        button.setId(ind);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.gravity = Gravity.CENTER;
        int sizeInDP = 15;

        int marginInDp = (int) TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP, sizeInDP, getResources()
                        .getDisplayMetrics());
        params.topMargin = marginInDp;
        button.setLayoutParams(params);

        // add generated button to view
        mainLayout.addView(button);

        // add our event handler (less memory than an anonymous inner class)
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                        /*
                        Intent i = new Intent(RoomSwitchesActivity.this, SwitchActivity.class);
                        i.putExtra("clientid", cursor.getString(0));
                        i.putExtra("serverid", devID);
                        i.putExtra("local", local);
                        startActivity(i);
                        */
                Intent intent = new Intent(RoomSwitchesActivity.this, RoomAutomationCurrentStatusService.class);
                intent.putExtra("serverid",devID);
                intent.putExtra("clientid",client);
                intent.putExtra("local", local);
                startService(intent);
            }
        });

    }

    @Override
    protected void onStop() {
        stopService(new Intent(this, RoomAutomationCurrentStatusService.class));
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        stopService(new Intent(this, RoomAutomationCurrentStatusService.class));
        super.onDestroy();
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


        // save the IP address and port for the next time the app is used
        editor.putString(PREF_IP, ipAddress); // set the ip address value to save
        editor.putString(PREF_PORT, portNumber); // set the port number to save
        editor.commit(); // save the IP and PORT

        EmailID = editEmail.getText().toString();
        PWD = editPWD.getText().toString();

        // get the pin number from the button that was clicked
        //if(view.getId()==buttonLogin.getId())
        {
            if (EmailID == "") {
                Toast.makeText(getApplicationContext(), "Please enter Registered EmailID", Toast.LENGTH_LONG).show();
            }
            if (PWD == "") {
                Toast.makeText(getApplicationContext(), "Please enter Password", Toast.LENGTH_LONG).show();
            }

        }
        //else if(view.getId()==buttonRegister.getId())
        {
            if (EmailID == "") {
                Toast.makeText(getApplicationContext(), "Please enter EmailID", Toast.LENGTH_LONG).show();
            }
            if (PWD == "") {
                Toast.makeText(getApplicationContext(), "Please enter Password", Toast.LENGTH_LONG).show();
            }

        }


    }

    private BroadcastReceiver mStatusLoadMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            // Get extra data included in the Intent
            String message = intent.getStringExtra("message");
            String clientid = intent.getStringExtra("clientid");
            Log.d("RoomAutomation", "Got message: " + message);

            //stopService(new Intent(context, RoomAutomationCurrentStatusService.class));
            Intent i = new Intent(RoomSwitchesActivity.this, SwitchActivity.class);
            i.putExtra("clientid", clientid);
            i.putExtra("serverid", devID);
            i.putExtra("local", local);
            startActivity(i);

        }
    };

    @Override
    protected void onResume() {
        // Register to receive message.
        LocalBroadcastManager.getInstance(this).registerReceiver(
                mStatusLoadMessageReceiver, new IntentFilter("status-load-done"));
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
        // Unregister since the activity is paused.
        LocalBroadcastManager.getInstance(this).unregisterReceiver(
                mStatusLoadMessageReceiver);

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
