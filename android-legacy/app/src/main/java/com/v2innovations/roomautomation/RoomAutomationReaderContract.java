package com.v2innovations.roomautomation;
import android.provider.BaseColumns;


/**
 * Created by virupaksha on 17-11-2016.
 */
public final class RoomAutomationReaderContract {
    // To prevent someone from accidentally instantiating the contract class,
    // give it an empty constructor.
    public RoomAutomationReaderContract() {}

    /* Inner class that defines the table contents */
    public static abstract class ServerEntry implements BaseColumns {
        public static final String TABLE_NAME = "servers";
        public static final String COLUMN_NAME_SERVER_ID = "serverid";
        public static final String COLUMN_NAME_PWD = "pwd";
        public static final String COLUMN_NAME_IP = "ip";
    }

    public static abstract class ClientEntry implements BaseColumns {
        public static final String TABLE_NAME = "clients";
        public static final String COLUMN_NAME_SERVER_ID = "serverid";
        public static final String COLUMN_NAME_CLIENT_ID = "clientid";
        public static final String COLUMN_NAME_PWD = "pwd";
        public static final String COLUMN_NAME_IP = "ip";
    }

    public static abstract class UserEntry implements BaseColumns {
        public static final String TABLE_NAME = "users";
        public static final String COLUMN_NAME_EMAIL_ID = "serverid";
        public static final String COLUMN_NAME_PWD = "pwd";
        public static final String COLUMN_NAME_DEVICE_ID = "deviceid";
    }

    public static abstract class SwitchEntry implements BaseColumns {
        public static final String TABLE_NAME = "switch";
        public static final String COLUMN_NAME_CLIENT_ID = "clientid";
        public static final String COLUMN_NAME_COMP_ID = "compid";
        public static final String COLUMN_NAME_MODE = "mode";
        public static final String COLUMN_NAME_STATUS = "status";
        public static final String COLUMN_NAME_VALUE = "value";
    }

    public static abstract class IPCredentialsEntry implements BaseColumns {
        public static final String TABLE_NAME = "ipcredentials";
        public static final String COLUMN_NAME_IP_ADDRESS = "ipaddress";
        public static final String COLUMN_NAME_PORT_NUM = "portnum";
        public static final String COLUMN_NAME_CON_STA = "consta";
    }

    public static abstract class ThemeEntry implements BaseColumns {
        public static final String TABLE_NAME = "theme";
        public static final String COLUMN_NAME_THEME_ID = "themeid";
        public static final String COLUMN_NAME_THEME_STA = "themesta";
        public static final String COLUMN_NAME_THEME_STR = "setting";
    }

    public static abstract class RemoteEntry implements BaseColumns {
        public static final String TABLE_NAME = "remote";
        public static final String COLUMN_NAME_MODULE_ID = "moduleid";
        public static final String COLUMN_NAME_ROOM_ID = "roomid";
        public static final String COLUMN_NAME_BUTTON_ID = "buttonid";
        //public static final String COLUMN_NAME_MODE = "mode";
        //public static final String COLUMN_NAME_STATUS = "status";
        public static final String COLUMN_NAME_VALUE = "value";
    }


}
