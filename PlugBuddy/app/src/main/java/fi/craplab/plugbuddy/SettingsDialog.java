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

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.support.v7.app.AlertDialog;
import android.view.View;
import android.widget.EditText;

/**
 *
 */
public class SettingsDialog extends DialogFragment {
    private SettingsSaveListener mListener;
    private SharedPreferences mSharedPrefs;

    private EditText mHostEdit;
    private EditText mPortEdit;
    private EditText mPlugIdEdit;

    private String mCurrentHost;
    private String mCurrentPort;
    private String mCurrentPlugId;

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);

        try {
            mListener = (SettingsSaveListener) context;
        } catch (ClassCastException e) {
            throw new ClassCastException(context.toString() + " must implement SettingsSaveListener");
        }

        mSharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
    }

    @NonNull
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        View rootView = getActivity().getLayoutInflater().inflate(R.layout.dialog_settings, null);

        mHostEdit   = (EditText) rootView.findViewById(R.id.host_edit);
        mPortEdit   = (EditText) rootView.findViewById(R.id.port_edit);
        mPlugIdEdit = (EditText) rootView.findViewById(R.id.plug_id_edit);

        mCurrentHost   = mSharedPrefs.getString(PlugBuddyApplication.PREF_WS_HOST, "");
        mCurrentPort   = mSharedPrefs.getString(PlugBuddyApplication.PREF_WS_PORT, "");
        mCurrentPlugId = mSharedPrefs.getString(PlugBuddyApplication.PREF_PLUG_ID, "");

        mHostEdit.setText(mCurrentHost);
        mPortEdit.setText(mCurrentPort);
        mPlugIdEdit.setText(mCurrentPlugId);

        builder.setView(rootView);
        builder.setNegativeButton("Cancel", null);
        builder.setPositiveButton("Save", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                boolean settingsChanged = false;
                if (!mHostEdit.getText().toString().equals(mCurrentHost)) {
                    settingsChanged = true;
                    mSharedPrefs.edit().putString(
                            PlugBuddyApplication.PREF_WS_HOST,
                            mHostEdit.getText().toString()).apply();
                }

                if (!mPortEdit.getText().toString().equals(mCurrentPort)) {
                    settingsChanged = true;
                    mSharedPrefs.edit().putString(
                            PlugBuddyApplication.PREF_WS_PORT,
                            mPortEdit.getText().toString()).apply();
                }

                if (!mPlugIdEdit.getText().toString().equals(mCurrentPlugId)) {
                    settingsChanged = true;
                    mSharedPrefs.edit().putString(
                            PlugBuddyApplication.PREF_PLUG_ID,
                            mPlugIdEdit.getText().toString()).apply();
                }

                if (settingsChanged) {
                    mListener.onSettingsChanged();
                }
            }
        });

        return builder.create();
    }

    interface SettingsSaveListener {
        void onSettingsChanged();
    }
}
