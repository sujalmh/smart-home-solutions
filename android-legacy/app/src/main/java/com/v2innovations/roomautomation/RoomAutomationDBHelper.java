package com.v2innovations.roomautomation;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

/**
 * Created by virupaksha on 17-11-2016.
 */

import com.v2innovations.roomautomation.RoomAutomationReaderContract.ServerEntry;
import com.v2innovations.roomautomation.RoomAutomationReaderContract.ClientEntry;
import com.v2innovations.roomautomation.RoomAutomationReaderContract.SwitchEntry;
import com.v2innovations.roomautomation.RoomAutomationReaderContract.UserEntry;

import java.io.Serializable;

public class RoomAutomationDBHelper extends SQLiteOpenHelper {
    // If you change the database schema, you must increment the database version.
    public static final int DATABASE_VERSION = 1;
    public static final String DATABASE_NAME = "RoomAutomation.db";
    public static final String TAG = "RoomAutomation";

    private static final String TEXT_TYPE = " TEXT";
    private static final String INT_TYPE = " INTEGER";
    private static final String COMMA_SEP = ",";
    private static final String UNIQUE_TYPE = " UNIQUE";
    private static final String SQL_CREATE_SERVERS =
            "CREATE TABLE " + ServerEntry.TABLE_NAME + " (" +
                    ServerEntry.COLUMN_NAME_SERVER_ID + TEXT_TYPE + UNIQUE_TYPE + COMMA_SEP +
                    ServerEntry.COLUMN_NAME_PWD + TEXT_TYPE + COMMA_SEP +
                    ServerEntry.COLUMN_NAME_IP + TEXT_TYPE +
                    " )";

    private static final String SQL_DELETE_SERVERS =
            "DROP TABLE IF EXISTS " + ServerEntry.TABLE_NAME;

    private static final String SQL_CREATE_CLIENTS =
            "CREATE TABLE " + ClientEntry.TABLE_NAME + " (" +
                    ClientEntry.COLUMN_NAME_SERVER_ID + TEXT_TYPE + COMMA_SEP +
                    ClientEntry.COLUMN_NAME_CLIENT_ID + TEXT_TYPE + UNIQUE_TYPE + COMMA_SEP +
                    ClientEntry.COLUMN_NAME_PWD + TEXT_TYPE + COMMA_SEP +
                    ClientEntry.COLUMN_NAME_IP + TEXT_TYPE +
                    " )";

    private static final String SQL_DELETE_CLIENTS =
            "DROP TABLE IF EXISTS " + ClientEntry.TABLE_NAME;

    private static final String SQL_CREATE_SWITCHES =
            "CREATE TABLE " + SwitchEntry.TABLE_NAME + " (" +
                    SwitchEntry.COLUMN_NAME_CLIENT_ID + TEXT_TYPE + COMMA_SEP +
                    SwitchEntry.COLUMN_NAME_COMP_ID + TEXT_TYPE + COMMA_SEP +
                    SwitchEntry.COLUMN_NAME_MODE + INT_TYPE + COMMA_SEP +
                    SwitchEntry.COLUMN_NAME_STATUS + INT_TYPE + COMMA_SEP +
                    SwitchEntry.COLUMN_NAME_VALUE + INT_TYPE +
                    " )";

    private static final String SQL_DELETE_SWITCH =
            "DROP TABLE IF EXISTS " + SwitchEntry.TABLE_NAME;

    private static final String SQL_CREATE_USERS =
            "CREATE TABLE " + UserEntry.TABLE_NAME + " (" +
                    UserEntry.COLUMN_NAME_EMAIL_ID + TEXT_TYPE + UNIQUE_TYPE + COMMA_SEP +
                    UserEntry.COLUMN_NAME_PWD + TEXT_TYPE + COMMA_SEP +
                    UserEntry.COLUMN_NAME_DEVICE_ID + TEXT_TYPE +
                    " )";

    private static final String SQL_DELETE_USERS =
            "DROP TABLE IF EXISTS " + ServerEntry.TABLE_NAME;

/*
    private static final String SQL_CREATE_REMOTE =
            "CREATE TABLE " + RemoteEntry.TABLE_NAME + " (" +
                    RemoteEntry.COLUMN_NAME_MODULE_ID + TEXT_TYPE + COMMA_SEP +
                    RemoteEntry.COLUMN_NAME_ROOM_ID + TEXT_TYPE + COMMA_SEP +
                    RemoteEntry.COLUMN_NAME_BUTTON_ID + TEXT_TYPE + COMMA_SEP +
                    //RemoteEntry.COLUMN_NAME_MODE + INT_TYPE + COMMA_SEP +
                    //RemoteEntry.COLUMN_NAME_STATUS + INT_TYPE + COMMA_SEP +
                    RemoteEntry.COLUMN_NAME_VALUE + TEXT_TYPE +
                    " )";

    private static final String SQL_DELETE_REMOTE =
            "DROP TABLE IF EXISTS " + RemoteEntry.TABLE_NAME;

    private static final String SQL_CREATE_IP_CREDENTIALS =
            "CREATE TABLE " + IPCredentialsEntry.TABLE_NAME + " (" +
                    IPCredentialsEntry.COLUMN_NAME_IP_ADDRESS + TEXT_TYPE + " PRIMARY KEY" + COMMA_SEP +
                    IPCredentialsEntry.COLUMN_NAME_PORT_NUM + TEXT_TYPE + COMMA_SEP +
                    IPCredentialsEntry.COLUMN_NAME_CON_STA + INT_TYPE +
                    " )";

    private static final String SQL_DELETE_IP_CREDENTIALS =
            "DROP TABLE IF EXISTS " + IPCredentialsEntry.TABLE_NAME;

    private static final String SQL_CREATE_THEMES =
            "CREATE TABLE " + ThemeEntry.TABLE_NAME + " (" +
                    ThemeEntry.COLUMN_NAME_THEME_ID + TEXT_TYPE + COMMA_SEP +
                    ThemeEntry.COLUMN_NAME_THEME_STA + INT_TYPE + COMMA_SEP +
                    ThemeEntry.COLUMN_NAME_THEME_STR + TEXT_TYPE +
                    " )";

    private static final String SQL_DELETE_THEMES =
            "DROP TABLE IF EXISTS " + ThemeEntry.TABLE_NAME;
*/

    public RoomAutomationDBHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    public void onCreate(SQLiteDatabase db) {
        //db.execSQL(SQL_CREATE_IP_CREDENTIALS);
        db.execSQL(SQL_CREATE_SERVERS);
        db.execSQL(SQL_CREATE_CLIENTS);
        db.execSQL(SQL_CREATE_SWITCHES);
        db.execSQL(SQL_CREATE_USERS);
        //db.execSQL(SQL_CREATE_REMOTE);
        //db.execSQL(SQL_CREATE_THEMES);
        Log.d(TAG, "HomeAutomationDBHelper onCreate function");
        //insertRooms();
        //insertRoom();


    }

    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        // This database is only a cache for online data, so its upgrade policy is
        // to simply to discard the data and start over
        db.execSQL(SQL_DELETE_SERVERS);
        db.execSQL(SQL_DELETE_CLIENTS);
        db.execSQL(SQL_DELETE_SWITCH);
        db.execSQL(SQL_DELETE_USERS);
        //db.execSQL(SQL_DELETE_REMOTE);
        //db.execSQL(SQL_DELETE_IP_CREDENTIALS);
        //db.execSQL(SQL_DELETE_THEMES);
        onCreate(db);
    }

    public void onDowngrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        onUpgrade(db, oldVersion, newVersion);
    }


    public  void deleteAllRecords() {
        SQLiteDatabase database = this.getWritableDatabase();
        database.execSQL("delete from "+ ServerEntry.TABLE_NAME);
        database.execSQL("delete from "+ ClientEntry.TABLE_NAME);
        database.execSQL("delete from " + SwitchEntry.TABLE_NAME);
        database.execSQL("delete from " + UserEntry.TABLE_NAME);
        //database.execSQL("delete from " + ThemeEntry.TABLE_NAME);
        //database.execSQL("delete from " + RemoteEntry.TABLE_NAME);
    }

    /*
    public  void resetAllRecords() {
        String rRoomsQuery = "SELECT * FROM "+RoomsEntry.TABLE_NAME;
        String rRoomQuery = "SELECT * FROM "+RoomEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();

        Cursor cursor = database.rawQuery(rRoomsQuery, null);
        if(cursor.moveToFirst()) {
            do {
                updateRoomsStatus(cursor.getString(0), 0);
            }while(cursor.moveToNext());
        }

        cursor = database.rawQuery(rRoomQuery,null);
        if(cursor.moveToFirst()) {
            do {
                updateModuleStatus(cursor.getString(1), cursor.getString(0), 0);
                updateModuleValue(cursor.getString(1),cursor.getString(0),0);
            }while(cursor.moveToNext());
        }

    }
    */

    public boolean recordsPresent() {
        String countQuery = "SELECT count(*) FROM "+ServerEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();
        Cursor cursor = database.rawQuery(countQuery, null);
        cursor.moveToFirst();
        if (cursor.getInt(0) > 0) return true;
        else return false;
    }

    public boolean clientsPresent() {
        String countQuery = "SELECT count(*) FROM "+ClientEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();
        Cursor cursor = database.rawQuery(countQuery, null);
        cursor.moveToFirst();
        if (cursor.getInt(0) > 0) return true;
        else return false;
    }

    public  void resetDB() {
        SQLiteDatabase db = this.getWritableDatabase();
        db.execSQL(SQL_DELETE_SERVERS);
        db.execSQL(SQL_DELETE_CLIENTS);
        db.execSQL(SQL_DELETE_SWITCH);
        db.execSQL(SQL_DELETE_USERS);
        //db.execSQL(SQL_DELETE_IP_CREDENTIALS);
        onCreate(db);
    }

    /*
    public void insertIPCredentials (String ipa, String port) {
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(IPCredentialsEntry.COLUMN_NAME_IP_ADDRESS, ipa);
        values.put(IPCredentialsEntry.COLUMN_NAME_PORT_NUM, port);
        database.insert(IPCredentialsEntry.TABLE_NAME, null, values);
        database.close();
    }

    public void deleteIPCredentials() {
        SQLiteDatabase database = this.getWritableDatabase();
        database.execSQL("delete from " + IPCredentialsEntry.TABLE_NAME);
    }


    public Cursor getIPCredentials() {
        Cursor cursor;
        String countQuery = "SELECT count(*) FROM "+IPCredentialsEntry.TABLE_NAME;
        String query = "Select * from "+IPCredentialsEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();

        cursor = database.rawQuery(countQuery, null);
        cursor.moveToFirst();
        if (cursor.getInt(0) > 0)
            cursor = database.rawQuery(query,null);
        else cursor = null;
        return cursor;

    }

    public int getConnectStatus(){
        String query = "Select "+IPCredentialsEntry.COLUMN_NAME_CON_STA+" from "+IPCredentialsEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();
        int stat = 0;
        Cursor cursor = database.rawQuery(query, null);
        if (cursor.moveToFirst()){
            do {
                stat=cursor.getInt(0);
            } while (cursor.moveToNext());
        }
        return stat;

    }

    public void updateConnectStatus (String ip, int s){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(IPCredentialsEntry.COLUMN_NAME_CON_STA, s);
        //queryValues.userId=database.insert("logins", null, values);
        //database.close();
        database.update(IPCredentialsEntry.TABLE_NAME, values, IPCredentialsEntry.COLUMN_NAME_IP_ADDRESS + " = ?", new String[]{String.valueOf(ip)});
        database.close();
    }
*/

    public void insertSwitch (String clientid){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(SwitchEntry.COLUMN_NAME_CLIENT_ID, clientid);
        values.put(SwitchEntry.COLUMN_NAME_COMP_ID, "Comp0");
        values.put(SwitchEntry.COLUMN_NAME_MODE, 1);
        values.put(SwitchEntry.COLUMN_NAME_STATUS, 0);
        values.put(SwitchEntry.COLUMN_NAME_VALUE, 1000);
        //database.insert(RoomEntry.TABLE_NAME, null, values);
        database.insertWithOnConflict(SwitchEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(SwitchEntry.COLUMN_NAME_CLIENT_ID, clientid);
        values.put(SwitchEntry.COLUMN_NAME_COMP_ID, "Comp1");
        values.put(SwitchEntry.COLUMN_NAME_MODE, 1);
        values.put(SwitchEntry.COLUMN_NAME_STATUS, 0);
        values.put(SwitchEntry.COLUMN_NAME_VALUE, 1000);
        //database.insert(RoomEntry.TABLE_NAME, null, values);
        database.insertWithOnConflict(SwitchEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(SwitchEntry.COLUMN_NAME_CLIENT_ID, clientid);
        values.put(SwitchEntry.COLUMN_NAME_COMP_ID, "Comp2");
        values.put(SwitchEntry.COLUMN_NAME_MODE, 1);
        values.put(SwitchEntry.COLUMN_NAME_STATUS, 0);
        values.put(SwitchEntry.COLUMN_NAME_VALUE, 1000);
        //database.insert(RoomEntry.TABLE_NAME, null, values);
        database.insertWithOnConflict(SwitchEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);
        /*
        values.put(SwitchEntry.COLUMN_NAME_CLIENT_ID, clientid);
        values.put(SwitchEntry.COLUMN_NAME_COMP_ID, "Comp3");
        values.put(SwitchEntry.COLUMN_NAME_MODE, 1);
        values.put(SwitchEntry.COLUMN_NAME_STATUS, 0);
        values.put(SwitchEntry.COLUMN_NAME_VALUE, 0);
        //database.insert(RoomEntry.TABLE_NAME, null, values);
        database.insertWithOnConflict(SwitchEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(SwitchEntry.COLUMN_NAME_CLIENT_ID, clientid);
        values.put(SwitchEntry.COLUMN_NAME_COMP_ID, "Comp4");
        values.put(SwitchEntry.COLUMN_NAME_MODE, 1);
        values.put(SwitchEntry.COLUMN_NAME_STATUS, 0);
        values.put(SwitchEntry.COLUMN_NAME_VALUE, 0);
        //database.insert(RoomEntry.TABLE_NAME, null, values);
        database.insertWithOnConflict(SwitchEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(SwitchEntry.COLUMN_NAME_CLIENT_ID, clientid);
        values.put(SwitchEntry.COLUMN_NAME_COMP_ID, "Comp5");
        values.put(SwitchEntry.COLUMN_NAME_MODE, 1);
        values.put(SwitchEntry.COLUMN_NAME_STATUS, 0);
        values.put(SwitchEntry.COLUMN_NAME_VALUE, 0);
        //database.insert(RoomEntry.TABLE_NAME, null, values);
        database.insertWithOnConflict(SwitchEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);
        */
        database.close();
        //return queryValues;
    }

    public boolean switchPresent(String clientid) {
        String query = "Select "+SwitchEntry.COLUMN_NAME_CLIENT_ID+" from "+SwitchEntry.TABLE_NAME+" where "+SwitchEntry.COLUMN_NAME_CLIENT_ID+" ='"+clientid+"'";
        SQLiteDatabase database = this.getReadableDatabase();
        boolean stat = false;
        Cursor cursor = database.rawQuery(query, null);
        if (cursor.moveToFirst()){
            do {
                stat=true;
                break;
            } while (cursor.moveToNext());
        }
        return stat;
    }

    public void updateSwitchMode (String clientid, String compid, int m){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(SwitchEntry.COLUMN_NAME_MODE, m);
        //queryValues.userId=database.insert("logins", null, values);
        //database.close();
        database.update(SwitchEntry.TABLE_NAME, values, SwitchEntry.COLUMN_NAME_CLIENT_ID + " = ?" + " and " + SwitchEntry.COLUMN_NAME_COMP_ID + " = ?", new String[]{String.valueOf(clientid), String.valueOf(compid)});
        //database.close();
    }

    public void updateModuleStatus (String clientid, String compid, int s){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(SwitchEntry.COLUMN_NAME_STATUS, s);
        //queryValues.userId=database.insert("logins", null, values);
        //database.close();
        database.update(SwitchEntry.TABLE_NAME, values, SwitchEntry.COLUMN_NAME_CLIENT_ID + " = ?" + " and " + SwitchEntry.COLUMN_NAME_COMP_ID + " = ?", new String[]{String.valueOf(clientid), String.valueOf(compid)});
        //database.close();
    }

    public void updateModule (String clientid, String compid, int m, int s, int v){
        SQLiteDatabase database = this.getWritableDatabase();
        /*
        Log.d("RoomAutomation","clientid: "+clientid);
        Log.d("RoomAutomation","compid: "+compid);
        Log.d("RoomAutomation","m: "+m);
        Log.d("RoomAutomation","s: "+s);
        Log.d("RoomAutomation","v: "+v);
        */
        ContentValues values = new ContentValues();
        values.put(SwitchEntry.COLUMN_NAME_MODE, m);
        values.put(SwitchEntry.COLUMN_NAME_STATUS, s);
        values.put(SwitchEntry.COLUMN_NAME_VALUE, v);
        //queryValues.userId=database.insert("logins", null, values);
        //database.close();
        database.update(SwitchEntry.TABLE_NAME, values, SwitchEntry.COLUMN_NAME_CLIENT_ID + " = ?" + " and " + SwitchEntry.COLUMN_NAME_COMP_ID + " = ?", new String[]{String.valueOf(clientid), String.valueOf(compid)});
        database.close();
    }

    public void updateModuleValue (String clientid, String compid, int v){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(SwitchEntry.COLUMN_NAME_VALUE, v);
        //queryValues.userId=database.insert("logins", null, values);
        //database.close();
        database.update(SwitchEntry.TABLE_NAME, values, SwitchEntry.COLUMN_NAME_CLIENT_ID + " = ?" + " and " + SwitchEntry.COLUMN_NAME_COMP_ID + " = ?", new String[]{String.valueOf(clientid), String.valueOf(compid)});
        //database.close();
    }

    public int getModuleStatus (String clientid, String compid){
        String query = "Select "+SwitchEntry.COLUMN_NAME_STATUS+" from "+SwitchEntry.TABLE_NAME+" where "+SwitchEntry.COLUMN_NAME_CLIENT_ID+" ='"+clientid+"'"+" AND "+SwitchEntry.COLUMN_NAME_COMP_ID+" ='"+compid+"'";
        SQLiteDatabase database = this.getReadableDatabase();
        int stat = 0;
        Cursor cursor = database.rawQuery(query, null);
        if (cursor.moveToFirst()){
            do {
                stat=cursor.getInt(0);
            } while (cursor.moveToNext());
        }
        return stat;
    }

    public int getModuleMode (String clientid, String compid){
        String query = "Select "+SwitchEntry.COLUMN_NAME_MODE+" from "+SwitchEntry.TABLE_NAME+" where "+SwitchEntry.COLUMN_NAME_CLIENT_ID+" ='"+clientid+"'"+" AND "+SwitchEntry.COLUMN_NAME_COMP_ID+" ='"+compid+"'";
        SQLiteDatabase database = this.getReadableDatabase();
        int mode = 0;
        Cursor cursor = database.rawQuery(query, null);
        if (cursor.moveToFirst()){
            do {
                mode = cursor.getInt(0);
            } while (cursor.moveToNext());
        }
        return mode;
    }

    public int getModuleValue (String clientid, String compid){
        String query = "Select "+SwitchEntry.COLUMN_NAME_VALUE+" from "+SwitchEntry.TABLE_NAME+" where "+SwitchEntry.COLUMN_NAME_CLIENT_ID+" ='"+clientid+"'"+" AND "+SwitchEntry.COLUMN_NAME_COMP_ID+" ='"+compid+"'";
        SQLiteDatabase database = this.getReadableDatabase();
        int val = 0;
        Cursor cursor = database.rawQuery(query, null);
        if (cursor.moveToFirst()){
            do {
                val=cursor.getInt(0);
            } while (cursor.moveToNext());
        }
        return val;
    }

    public Cursor getAllServers() {
        String query = "Select "+ServerEntry.COLUMN_NAME_SERVER_ID + " from "+ServerEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();
        Cursor cursor = database.rawQuery(query, null);
        return cursor;

    }

    public Cursor getAllClients() {
        String query = "Select "+ClientEntry.COLUMN_NAME_CLIENT_ID + " from "+ClientEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();
        Cursor cursor = database.rawQuery(query, null);
        return cursor;

    }

    public Cursor getAllComponents(String clientid) {
        String query = "Select * from "+SwitchEntry.TABLE_NAME+" where "+SwitchEntry.COLUMN_NAME_CLIENT_ID+" ='"+clientid+"'";
        SQLiteDatabase database = this.getReadableDatabase();
        Cursor cursor = database.rawQuery(query, null);
        return cursor;

    }

    public Cursor getUser() {
        String query = "Select * from "+UserEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();
        Cursor cursor = database.rawQuery(query, null);
        return cursor;

    }


    /*
    public Cursor getRoomModulesID(String id) {
        String query = "Select "+RoomEntry.COLUMN_NAME_MODULE_ID + " from "+RoomEntry.TABLE_NAME+" where "+RoomEntry.COLUMN_NAME_ROOM_ID+" ='"+id+"'";
        SQLiteDatabase database = this.getReadableDatabase();
        Cursor cursor = database.rawQuery(query, null);
        return cursor;
    }

    public Cursor getRoomModules(String id) {
        String query = "Select "+RoomEntry.COLUMN_NAME_STATUS + " from "+RoomEntry.TABLE_NAME+" where "+RoomEntry.COLUMN_NAME_ROOM_ID+" ='"+id+"'";
        SQLiteDatabase database = this.getReadableDatabase();
        Cursor cursor = database.rawQuery(query, null);
        return cursor;

    }
    */
    public void displaySwitchComps() {
        String query = "Select * from "+SwitchEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();
        Cursor cursor = database.rawQuery(query, null);

        if (cursor.moveToFirst()){
            do {
                Log.d(TAG,"clientid = "+cursor.getString(0));
                Log.d(TAG,"compid = "+cursor.getString(1));
                Log.d(TAG,"status = "+cursor.getInt(2));
                Log.d(TAG,"value = "+cursor.getInt(3));
            } while (cursor.moveToNext());
        }

    }


    public void insertServer (String serverid, String pw){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(ServerEntry.COLUMN_NAME_SERVER_ID, serverid);
        values.put(ServerEntry.COLUMN_NAME_PWD, pw);
        values.put(ServerEntry.COLUMN_NAME_IP, "192.168.4.100");
        //values.put(ServerEntry.COLUMN_NAME_IP, "192.168.4.1");
        //database.insert(RoomsEntry.TABLE_NAME, null, values);
        //database.execSQL("delete from " + ServerEntry.TABLE_NAME + " where " + ServerEntry.COLUMN_NAME_SERVER_ID + " not in (SELECT MIN(" + ServerEntry.COLUMN_NAME_SERVER_ID + ") FROM " + ServerEntry.TABLE_NAME + " GROUP BY " + ServerEntry.COLUMN_NAME_SERVER_ID + ")");
        //database.execSQL("delete from " + ServerEntry.TABLE_NAME + " where rowid not in (SELECT MIN(rowid) FROM " + ServerEntry.TABLE_NAME + " GROUP BY " + ServerEntry.COLUMN_NAME_SERVER_ID + ")");
        database.insertWithOnConflict(ServerEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);
        //database.insert(ServerEntry.TABLE_NAME, null, values);

        database.close();
        //return queryValues;
    }

    public void insertUser (String emailid, String pw, String devid){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(UserEntry.COLUMN_NAME_EMAIL_ID, emailid);
        values.put(UserEntry.COLUMN_NAME_PWD, pw);
        values.put(UserEntry.COLUMN_NAME_DEVICE_ID, devid);
        //database.insert(RoomsEntry.TABLE_NAME, null, values);
        database.insertWithOnConflict(UserEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);
        //database.insert(UserEntry.TABLE_NAME, null, values);
        database.close();
        //return queryValues;
    }

    public String getEmailID (){
        String query = "Select "+UserEntry.COLUMN_NAME_EMAIL_ID+" from "+UserEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();
        String code = "";
        Cursor cursor = database.rawQuery(query, null);
        if (cursor.moveToFirst()){
            do {
                code=cursor.getString(0);
            } while (cursor.moveToNext());
        }
        return code;
    }

    public String getServerID (){
        String query = "Select "+ServerEntry.COLUMN_NAME_SERVER_ID+" from "+ServerEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();
        String code = "";
        Cursor cursor = database.rawQuery(query, null);
        if (cursor.moveToFirst()){
            do {
                code=cursor.getString(0);
            } while (cursor.moveToNext());
        }
        return code;
    }

    public String getServerIP (String id){
        String query = "Select "+ServerEntry.COLUMN_NAME_IP+" from "+ServerEntry.TABLE_NAME+" where "+ServerEntry.COLUMN_NAME_SERVER_ID+" ='"+id+"'";
        SQLiteDatabase database = this.getReadableDatabase();
        String code = "";
        Cursor cursor = database.rawQuery(query, null);
        if (cursor.moveToFirst()){
            do {
                code=cursor.getString(0);
            } while (cursor.moveToNext());
        }
        return code;
    }

    public void updateServerIP (String serverid, String ip){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(ServerEntry.COLUMN_NAME_IP, ip);
        database.update(ServerEntry.TABLE_NAME, values, ServerEntry.COLUMN_NAME_SERVER_ID + " = ?", new String[]{String.valueOf(serverid)});
        //database.close();
    }

    public void insertClient (String serverid, String clientid, String pw, String ip){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(ClientEntry.COLUMN_NAME_SERVER_ID, serverid);
        values.put(ClientEntry.COLUMN_NAME_CLIENT_ID, clientid);
        values.put(ClientEntry.COLUMN_NAME_PWD, pw);
        values.put(ClientEntry.COLUMN_NAME_IP, ip);
        //database.insert(RoomsEntry.TABLE_NAME, null, values);
        database.insertWithOnConflict(ClientEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);
        //database.insert(ClientEntry.TABLE_NAME, null, values);
        database.close();
        //return queryValues;
    }
    /*
    public void updateRoomsStatus (String id, int s){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(RoomsEntry.COLUMN_NAME_STATUS, s);
        //queryValues.userId=database.insert("logins", null, values);
        //database.close();
        database.update(RoomsEntry.TABLE_NAME, values, RoomsEntry.COLUMN_NAME_ROOM_ID + " = ?", new String[]{String.valueOf(id)});
        //database.close();
    }

    public int getRoomsStatus (String id){
        String query = "Select "+RoomsEntry.COLUMN_NAME_STATUS+" from "+RoomsEntry.TABLE_NAME+" where "+RoomsEntry.COLUMN_NAME_ROOM_ID+" ='"+id+"'";
        SQLiteDatabase database = this.getReadableDatabase();
        int stat = 0;
        Cursor cursor = database.rawQuery(query, null);
        if (cursor.moveToFirst()){
            do {
                stat=cursor.getInt(0);
            } while (cursor.moveToNext());
        }
        return stat;
    }

    public Cursor getAllRooms() {
        String query = "Select " + RoomsEntry.COLUMN_NAME_ROOM_ID + " from "+RoomsEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();
        Cursor cursor = database.rawQuery(query, null);
        return cursor;

    }

    public void displayRooms() {
        String query = "Select * from "+RoomsEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();
        Cursor cursor = database.rawQuery(query, null);

        if (cursor.moveToFirst()){
            do {
                Log.d(TAG,"roomid = "+cursor.getString(0));
                Log.d(TAG,"status = "+cursor.getInt(1));
            } while (cursor.moveToNext());
        }

    }

    public void insertThemes (){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(ThemeEntry.COLUMN_NAME_THEME_ID, "NIGHT");
        values.put(ThemeEntry.COLUMN_NAME_THEME_STA, 0);
        values.put(ThemeEntry.COLUMN_NAME_THEME_STR, "BR1;LI1;0;1;:BR1;FA1;0;4;:KI1;AP1;0;1;:KI1;LI1;0;2;");
        //database.insert(RoomsEntry.TABLE_NAME, null, values);
        database.insertWithOnConflict(ThemeEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(ThemeEntry.COLUMN_NAME_THEME_ID, "MORN");
        values.put(ThemeEntry.COLUMN_NAME_THEME_STA, 0);
        values.put(ThemeEntry.COLUMN_NAME_THEME_STR, "BR1;MU1;0;4;:BR1;FA1;0;1;:KI1;AP1;0;1;:KI1;LI1;0;10;");
        //database.insert(RoomsEntry.TABLE_NAME, null, values);
        database.insertWithOnConflict(ThemeEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(ThemeEntry.COLUMN_NAME_THEME_ID, "OUT");
        values.put(ThemeEntry.COLUMN_NAME_THEME_STA, 0);
        values.put(ThemeEntry.COLUMN_NAME_THEME_STR, "KI1;AP2;0;1:KI1;AP1;0;1;");
        //database.insert(RoomsEntry.TABLE_NAME, null, values);
        database.insertWithOnConflict(ThemeEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(ThemeEntry.COLUMN_NAME_THEME_ID, "KIDS");
        values.put(ThemeEntry.COLUMN_NAME_THEME_STA, 0);
        values.put(ThemeEntry.COLUMN_NAME_THEME_STR, "LR1;FA1;0;4;:LR1;TV1;0;40;");
        //database.insert(RoomsEntry.TABLE_NAME, null, values);
        database.insertWithOnConflict(ThemeEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        database.close();
        //return queryValues;
    }

    public int getThemeStatus (String id){
        String query = "Select "+ThemeEntry.COLUMN_NAME_THEME_STA+" from "+ThemeEntry.TABLE_NAME+" where "+ThemeEntry.COLUMN_NAME_THEME_ID+" ='"+id+"'";
        SQLiteDatabase database = this.getReadableDatabase();
        int stat = 0;
        Cursor cursor = database.rawQuery(query, null);
        if (cursor.moveToFirst()){
            do {
                stat=cursor.getInt(0);
            } while (cursor.moveToNext());
        }
        return stat;
    }

    public void updateThemeStatus (String id, int s){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(ThemeEntry.COLUMN_NAME_THEME_STA, s);
        database.update(ThemeEntry.TABLE_NAME, values, ThemeEntry.COLUMN_NAME_THEME_ID + " = ?", new String[]{String.valueOf(id)});
        //database.close();
    }

    public String getThemeSetting(String id){
        String query = "Select "+ThemeEntry.COLUMN_NAME_THEME_STR+" from "+ThemeEntry.TABLE_NAME+" where "+ThemeEntry.COLUMN_NAME_THEME_ID+" ='"+id+"'";
        SQLiteDatabase database = this.getReadableDatabase();
        String setting = "";
        Cursor cursor = database.rawQuery(query, null);
        if (cursor.moveToFirst()){
            do {
                setting=cursor.getString(0);
            } while (cursor.moveToNext());
        }
        return setting;
    }

    public void updateThemeSetting (String id, String s){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(ThemeEntry.COLUMN_NAME_THEME_STR, s);
        database.update(ThemeEntry.TABLE_NAME, values, ThemeEntry.COLUMN_NAME_THEME_ID + " = ?", new String[]{String.valueOf(id)});
        //database.close();
    }

    public int isAnyThemeEnabled (){
        String query = "Select "+ThemeEntry.COLUMN_NAME_THEME_STA+" from "+ThemeEntry.TABLE_NAME;
        SQLiteDatabase database = this.getReadableDatabase();
        int stat = 0;
        Cursor cursor = database.rawQuery(query, null);
        if (cursor.moveToFirst()){
            do {
                if(cursor.getInt(0) == 1){
                    stat = 1;
                    break;
                }
            } while (cursor.moveToNext());
        }
        return stat;
    }

    public void insertRemote (){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "PWR");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "MUT");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "VUP");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "VDN");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "ONE");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "TWO");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "THR");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "FOU");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "FIV");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "SIX");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "SEV");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "EIG");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "NIN");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "ZER");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "CUP");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "CDN");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "BAK");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "MEN");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "OUP");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "ODN");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "OLT");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "ORT");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "TV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "OOK");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "PWR");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "MUT");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "VUP");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "VDN");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "ONE");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "TWO");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "THR");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "FOU");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "FIV");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "SIX");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "SEV");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "EIG");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "NIN");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "ZER");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "CUP");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "CDN");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "SB1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "BAK");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "AC1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "PWR");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "AC1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "MOD");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "AC1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "FAN");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "AC1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "TDN");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "AC1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "TUP");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "AC1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "HSW");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "AC1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "VSW");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "DV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "PWR");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "DV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "PAU");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "DV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "PLA");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "DV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "PRE");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "DV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "NXT");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "DV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "FWD");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "DV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "REW");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "DV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "OPN");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "DV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "BAK");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "DV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "MUT");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "DV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "STO");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "DV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "VUP");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        values.put(RemoteEntry.COLUMN_NAME_ROOM_ID, "KI1");
        values.put(RemoteEntry.COLUMN_NAME_MODULE_ID, "DV1");
        values.put(RemoteEntry.COLUMN_NAME_BUTTON_ID, "VDN");
        values.put(RemoteEntry.COLUMN_NAME_VALUE, "");
        database.insertWithOnConflict(RemoteEntry.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_IGNORE);

        database.close();
        //return queryValues;
    }

    public void updateButtonValue (String roomid, String moduleid, String buttonid, String v){
        SQLiteDatabase database = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(RemoteEntry.COLUMN_NAME_VALUE, v);
        //queryValues.userId=database.insert("logins", null, values);
        //database.close();
        database.update(RemoteEntry.TABLE_NAME, values, RemoteEntry.COLUMN_NAME_ROOM_ID + " = ?" + " and " + RemoteEntry.COLUMN_NAME_MODULE_ID + " = ?" + " and " + RemoteEntry.COLUMN_NAME_BUTTON_ID + " = ?", new String[] {String.valueOf(roomid),String.valueOf(moduleid),String.valueOf(buttonid)});
        //database.close();
    }


    public String getButtonValue (String roomid, String moduleid, String buttonid){
        String query = "Select "+RemoteEntry.COLUMN_NAME_VALUE+" from "+RemoteEntry.TABLE_NAME+" where "+RemoteEntry.COLUMN_NAME_ROOM_ID+" ='"+roomid+"'"+" AND "+RemoteEntry.COLUMN_NAME_MODULE_ID+" ='"+moduleid+"'"+" AND "+RemoteEntry.COLUMN_NAME_BUTTON_ID+" ='"+buttonid+"'";
        SQLiteDatabase database = this.getReadableDatabase();
        String code = "";
        Cursor cursor = database.rawQuery(query, null);
        if (cursor.moveToFirst()){
            do {
                code=cursor.getString(0);
            } while (cursor.moveToNext());
        }
        return code;
    }
*/

}