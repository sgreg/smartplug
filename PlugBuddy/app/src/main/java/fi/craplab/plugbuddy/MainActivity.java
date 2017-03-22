/*
 * PlugBuddy - the CrapLab SmartPlug companion app
 *
 * Copyright (C) 2017 Sven Gregori <sven@craplab.fi>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

package fi.craplab.plugbuddy;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Toast;

import com.joanzapata.iconify.IconDrawable;
import com.joanzapata.iconify.fonts.FontAwesomeIcons;
import com.joanzapata.iconify.widget.IconTextView;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Random;

import de.tavendo.autobahn.WebSocketConnection;
import de.tavendo.autobahn.WebSocketException;
import de.tavendo.autobahn.WebSocketHandler;

/**
 * Main Activity, doing basically everything there is to do ..apart from displaying
 * the notifications, see {@link PlugNotification} for that.
 */
public class MainActivity extends AppCompatActivity implements SettingsDialog.SettingsSaveListener {
    private static final String TAG = MainActivity.class.getSimpleName();

    private SharedPreferences mSharedPrefs;

    private static final String[] UNBUTTON_TEXTS = {
            "This is not a button.",
            "Don't you touch me!",
            "Do I look like a button to you?!",
            "Nope."
    };
    private static final Random mRandom = new Random();

    private WebSocketConnection mConnection;
    private IconTextView mPowerIcon;
    private boolean mStatusReceived = false;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mPowerIcon = (IconTextView) findViewById(R.id.power_icon);
        setButtonColor(R.color.disconnected);

        mPowerIcon.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Toast.makeText(
                        MainActivity.this,
                        UNBUTTON_TEXTS[mRandom.nextInt(UNBUTTON_TEXTS.length)],
                        Toast.LENGTH_SHORT).show();
            }
        });

        mSharedPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        webSocketConnect();
    }

    @Override
    protected void onDestroy() {
        if (mConnection.isConnected()) {
            mConnection.disconnect();
        }
        super.onDestroy();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        setTitle("");
        getMenuInflater().inflate(R.menu.menu_main, menu);

        menu.findItem(R.id.settings).setIcon(
                new IconDrawable(this, FontAwesomeIcons.fa_gear)
                        .colorRes(R.color.settingsIcon)
                        .actionBarSize());

        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.settings) {
            SettingsDialog dialog = new SettingsDialog();
            dialog.show(getSupportFragmentManager(), "settingsDialog");
        }
        return super.onOptionsItemSelected(item);
    }

    private void webSocketConnect() {
        mConnection = new WebSocketConnection();

        try {
            mConnection.connect(getWebsocketUri(), mSocketHandler);
        } catch (WebSocketException e) {
            e.printStackTrace();
        }
    }

    private String getWebsocketUri() {
        return String.format("ws://%s:%s",
                mSharedPrefs.getString(PlugBuddyApplication.PREF_WS_HOST, ""),
                mSharedPrefs.getString(PlugBuddyApplication.PREF_WS_PORT, ""));
    }


    private void setButtonColor(int colorRes) {
        mPowerIcon.setTextColor(ContextCompat.getColor(this, colorRes));
    }

    private final WebSocketHandler mSocketHandler = new WebSocketHandler() {
        @Override
        public void onOpen() {
            Log.d(TAG, "Status: Connected to " + getWebsocketUri());
            setButtonColor(R.color.unplugged);

            try {
                JSONObject json = new JSONObject();
                json.put("status", mSharedPrefs.getString(
                        PlugBuddyApplication.PREF_PLUG_ID, "0xb00bf0cc"));
                mConnection.sendTextMessage(json.toString());

            } catch (JSONException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void onTextMessage(String payload) {
            try {
                JSONObject json = new JSONObject(payload);
                String clientId = json.getString("id");
                int value = json.getInt("value");

                Log.d(TAG, "Received new value for " + clientId + ": " + value);
                setButtonColor(((value == 1) ? R.color.plugged : R.color.unplugged));

                // make sure status request on startup won't trigger notification
                if (mStatusReceived) {
                    PlugNotification.notify(MainActivity.this, value);
                } else {
                    mStatusReceived = true;
                }

            } catch (JSONException e) {
                Log.e(TAG, "Failed to parse received JSON '" + payload + "'", e);
            }
        }

        @Override
        public void onClose(int code, String reason) {
            setButtonColor(R.color.disconnected);
            Log.i(TAG, "Connection lost: " + reason);
        }
    };

    @Override
    public void onSettingsChanged() {
        Log.d(TAG, "settings saved, yo");
        if (mConnection.isConnected()) {
            mConnection.disconnect();
            mConnection = null;
        }

        webSocketConnect();
    }
}
