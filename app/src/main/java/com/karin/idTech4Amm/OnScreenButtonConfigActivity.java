package com.karin.idTech4Amm;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.view.View;
import android.widget.Toast;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.preference.PreferenceManager;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.Utility;
import com.karin.idTech4Amm.sys.PreferenceKey;
import com.karin.idTech4Amm.sys.Theme;
import com.karin.idTech4Amm.ui.ArrayAdapter_base;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.karin.KStr;

import android.view.ViewGroup;
import java.util.List;
import java.util.ArrayList;
import android.widget.ListView;
import android.widget.ImageView;
import java.util.Arrays;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.widget.AdapterView;
import android.widget.CheckBox;
import android.widget.Spinner;
import android.content.SharedPreferences;
import android.util.SparseBooleanArray;
import android.widget.Adapter;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Set;
import java.util.LinkedHashSet;
import android.content.res.Resources;

/**
 * On-screen button config
 */
@SuppressLint({"ApplySharedPref",})
public class OnScreenButtonConfigActivity extends Activity
{
    private List<OnScreenButton> m_list = new ArrayList<>();
    private OnScreenButtonAdapter m_adapter;
    private ViewHolder V;
    private Map<Integer, String> m_styleMap;
    private Map<Integer, String> m_sliderStyleMap;
    private Map<Integer, String> m_keyMap;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Q3ELang.Locale(this);

        boolean o = PreferenceManager.getDefaultSharedPreferences(this).getBoolean(PreferenceKey.LAUNCHER_ORIENTATION, false);
        ContextUtility.SetScreenOrientation(this, o ? 0 : 1);

        m_styleMap = BuildKeyValueMapFromResource(R.array.onscreen_button_style_values, R.array.onscreen_button_style_labels);
        m_sliderStyleMap = BuildKeyValueMapFromResource(R.array.onscreen_slider_style_values, R.array.onscreen_slider_style_labels);
        // Map<Integer, String> m_typeMap = BuildKeyValueMapFromResource(R.array.onscreen_type_values, R.array.onscreen_type_labels);
        m_keyMap = BuildKeyValueMapFromResource(R.array.key_map_codes, R.array.key_map_names);

        Theme.SetTheme(this, false);
        setContentView(R.layout.onscreen_button_config_page);
        V = new ViewHolder();
        m_adapter = new OnScreenButtonAdapter(R.layout.onscreen_button_config_list_delegate, m_list);
        V.lst.setAdapter(m_adapter);
        V.lst.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView listView, View view, int position, long id)
            {
                OpenConfigDialog((OnScreenButton)listView.getAdapter().getItem(position));
            }
        });

        SetupUI();
    }

    private void SetupUI()
    {
        LoadConfig();
    }

    private void Reset()
    {
        List<OnScreenButton> tmp = new ArrayList<>();
        if(null != m_list)
        {
            tmp.addAll(m_list);
            m_list.clear();
        }
        m_adapter.notifyDataSetChanged();
        for (OnScreenButton onScreenButton : tmp)
            onScreenButton.Release();
    }

    private void LoadConfig()
    {
        Reset();
        Q3EInterface q3ei = Q3EUtils.q3ei;
        for(int i = 0; i < Q3EGlobals.UI_SIZE; i++)
        {
            int type = q3ei.type_table[i];
            if(type == Q3EGlobals.TYPE_JOYSTICK || type == Q3EGlobals.TYPE_DISC)
                continue;
            if(i == Q3EGlobals.UI_KBD || i == Q3EGlobals.UI_CONSOLE)
                continue;
            int[] arr = new int[4];
            System.arraycopy(q3ei.arg_table, i * 4, arr, 0, arr.length);
            m_list.add(new OnScreenButton(i, type, arr, q3ei.texture_table[i], Q3EGlobals.CONTROLS_NAMES[i]));
        }
        m_adapter.notifyDataSetChanged();
    }

    private void RestoreConfig()
    {
        ContextUtility.Confirm(this, Q3ELang.tr(this, R.string.warning), Q3ELang.tr(this, R.string.reset_onscreen_button_config), new Runnable() {
                public void run()
                {
                    SharedPreferences.Editor preferences = PreferenceManager.getDefaultSharedPreferences(OnScreenButtonConfigActivity.this).edit();
                    preferences.remove(Q3EPreference.ONSCREEN_BUTTON);
                    preferences.commit();
                    Q3EInterface.RestoreDefaultOnScreenConfig(Q3EUtils.q3ei.arg_table, Q3EUtils.q3ei.type_table);
                    LoadConfig();
                    Toast.makeText(OnScreenButtonConfigActivity.this, R.string.onscreen_button_config_has_reset, Toast.LENGTH_SHORT).show();
                }
            }, null, null, null);
    }

    public Map<Integer, String> BuildKeyValueMapFromResource(int keysId, int valuesId)
    {
        int[] keys = getResources().getIntArray(keysId);
        String[] values = getResources().getStringArray(valuesId);
        Map<Integer, String> res = new LinkedHashMap<>();
        for(int i = 0; i < keys.length; i++)
        {
            res.put(keys[i], values[i]);
        }
        return res;
    }

    private void SaveConfig()
    {
        int[] args = Q3EUtils.q3ei.arg_table;
        int[] type = Q3EUtils.q3ei.type_table;
        Set<String> list = new LinkedHashSet<>();
        for(OnScreenButton btn : m_list)
        {
            int[] a = btn.ToArray();
            System.arraycopy(a, 0, args, btn.index * 4, a.length);
            type[btn.index] = btn.type;
            list.add(btn.toString());
        }
        PreferenceManager.getDefaultSharedPreferences(this).edit().putStringSet(Q3EPreference.ONSCREEN_BUTTON, list).commit();
        m_adapter.notifyDataSetChanged();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.onscreen_button_config_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        int itemId = item.getItemId();
        if (itemId == R.id.onscreen_button_config_weapon_panel)
        {
            OpenWeaponPanelKeysSetting();
        }
        else if (itemId == R.id.onscreen_button_config_reset)
        {
            RestoreConfig();
        }
        else
        {
            return super.onOptionsItemSelected(item);
        }
        return true;
    }

    private class OnScreenButton
    {
        public int index;
        public int[] key;
        public String name;
        public boolean hold;
        public int style;
        public int type;
        public Bitmap texture;
        public String keyName;

        public OnScreenButton(int index, int type, int[] arr, String texture, String name)
        {
            this.index = index;
            this.type = type;
            this.name = name;
            int keys = 3;
            this.key = new int[keys];
            System.arraycopy(arr, 0, this.key, 0, keys);
            switch(type)
            {
                case Q3EGlobals.TYPE_BUTTON:
                    this.hold = arr[1] == Q3EGlobals.ONSCRREN_BUTTON_CAN_HOLD;
                    this.style = arr[2];
                    this.key[0] = arr[0];
                    break;
                case Q3EGlobals.TYPE_SLIDER:
                    this.style = arr[3];
                    this.key[0] = arr[0];
                    this.key[1] = arr[1];
                    this.key[2] = arr[2];
                    break;
                default:
                    break;
            }
            if(null != texture)
            {
                try
                {
                    String[] split = texture.split(";");
                    String texturePath = split[0];
                    this.texture = BitmapFactory.decodeStream(getAssets().open(texturePath));
                }
                catch(Exception e)
                {
                    e.printStackTrace();
                }
            }
            Update();
        }

        private void Update()
        {
            List<String> list = new ArrayList<>();
            switch(type)
            {
                case Q3EGlobals.TYPE_BUTTON:
                    list.add(m_keyMap.get(key[0]));
                    break;
                case Q3EGlobals.TYPE_SLIDER:
                    list.add(m_keyMap.get(key[0]));
                    list.add(m_keyMap.get(key[1]));
                    list.add(m_keyMap.get(key[2]));
                    break;
                default:
                    break;
            }
            keyName = KStr.Join(list, "  ");
        }

        private int[] ToArray()
        {
            int[] res = new int[4];
            switch(type)
            {
                case Q3EGlobals.TYPE_BUTTON:
                    res[1] = hold ? Q3EGlobals.ONSCRREN_BUTTON_CAN_HOLD : Q3EGlobals.ONSCRREN_BUTTON_NOT_HOLD;
                    res[2] = style;
                    res[0] = key[0];
                    break;
                case Q3EGlobals.TYPE_SLIDER:
                    res[0] = key[0];
                    res[1] = key[1];
                    res[2] = key[2];
                    res[3] = style;
                    break;
                default:
                break;
            }
            return res;
        }

        public String toString()
        {
            int[] args = ToArray();
            StringBuilder sb = new StringBuilder();
            sb.append(index)
            .append(":")
            .append(type)
            .append(";")
            ;
            for(int i = 0; i < args.length; i++)
            {
                sb.append(args[i]);
                if(i < args.length - 1)
                    sb.append(",");
            }
            return sb.toString();
        }

        public void Release()
        {
            if(null != texture)
            {
                texture.recycle();
                texture = null;
            }
        }
    }

    private class OnScreenButtonAdapter extends ArrayAdapter_base<OnScreenButton>
    {
        public OnScreenButtonAdapter(int resource, List<OnScreenButton> list)
        {
            super(OnScreenButtonConfigActivity.this, resource, list);
        }

        @Override
        public View GenerateView(int position, View view, ViewGroup parent, OnScreenButton data)
        {
            ImageView image = view.findViewById(R.id.onscreen_button_config_list_delegate_image);
            image.setImageBitmap(data.texture);
            TextView nameTextView = view.findViewById(R.id.onscreen_button_config_list_delegate_name);
            nameTextView.setText(data.name);
            TextView keyNameTextView = view.findViewById(R.id.onscreen_button_config_list_delegate_keys);
            keyNameTextView.setText(data.keyName);

            TextView holdTextView = view.findViewById(R.id.onscreen_button_config_list_delegate_hold);
            TextView styleTextView = view.findViewById(R.id.onscreen_button_config_list_delegate_style);
            switch(data.type)
            {
                case Q3EGlobals.TYPE_BUTTON:
                    holdTextView.setText(data.hold ? R.string.toggle : R.string.button);
					styleTextView.setText(m_styleMap.get(data.style) != null ? m_styleMap.get(data.style) : "");
                    break;
                case Q3EGlobals.TYPE_SLIDER:
                    holdTextView.setText(R.string.slider);
                    styleTextView.setText(m_sliderStyleMap.get(data.style) != null ? m_sliderStyleMap.get(data.style) : "");
                    break;
                default:
                    break;
            }
            return view;
        }
    }

    private class ViewHolder
    {
        public ListView lst;

        public ViewHolder()
        {
            lst = findViewById(R.id.onscreen_button_config_page_list);
        }
    }

    private void OpenConfigDialog(final OnScreenButton btn)
    {
        Resources rsc = getResources();
        final int[] styleValues = rsc.getIntArray(R.array.onscreen_button_style_values);
        final int[] typeValues = rsc.getIntArray(R.array.onscreen_type_values);
        final int[] keyCodes = rsc.getIntArray(R.array.key_map_codes);
        final int[] sliderStyleValues = rsc.getIntArray(R.array.onscreen_slider_style_values);
        View view = getLayoutInflater().inflate(R.layout.onscreen_button_config_dialog, null, false);
        final CheckBox holdCheckBox = view.findViewById(R.id.onscreen_button_config_dialog_hold);
        final Spinner styleSpinner = view.findViewById(R.id.onscreen_button_config_dialog_style);
        final Spinner sliderStyleSpinner = view.findViewById(R.id.onscreen_button_config_dialog_slider_style);
        final Spinner typeSpinner = view.findViewById(R.id.onscreen_button_config_dialog_type);
        final Spinner keySpinner = view.findViewById(R.id.onscreen_button_config_dialog_key);
        final Spinner key2Spinner = view.findViewById(R.id.onscreen_button_config_dialog_key2);
        final Spinner key3Spinner = view.findViewById(R.id.onscreen_button_config_dialog_key3);
        final View styleLayout = view.findViewById(R.id.onscreen_button_config_dialog_button_style_layout);
        final View holdLayout = view.findViewById(R.id.onscreen_button_config_dialog_button_hold_layout);
        final View keyLayout = view.findViewById(R.id.onscreen_button_config_dialog_key_layout);
        final View key2Layout = view.findViewById(R.id.onscreen_button_config_dialog_key2_layout);
        final View key3Layout = view.findViewById(R.id.onscreen_button_config_dialog_key3_layout);
        final View sliderStyleLayout = view.findViewById(R.id.onscreen_button_config_dialog_slider_style_layout);
        final Runnable runnable = new Runnable() {
            public void run() {
                typeSpinner.setSelection(Utility.ArrayIndexOf(typeValues, btn.type));
                switch(btn.type)
                {
                    case Q3EGlobals.TYPE_BUTTON:
                        holdLayout.setVisibility(View.VISIBLE);
                        styleLayout.setVisibility(View.VISIBLE);
                        keyLayout.setVisibility(View.VISIBLE);
                        key2Layout.setVisibility(View.GONE);
                        key3Layout.setVisibility(View.GONE);
                        sliderStyleLayout.setVisibility(View.GONE);
                        holdCheckBox.setChecked(btn.hold);
                        styleSpinner.setSelection(Utility.ArrayIndexOf(styleValues, btn.style));
                        keySpinner.setSelection(Utility.ArrayIndexOf(keyCodes, btn.key[0]));
                        break;
                    case Q3EGlobals.TYPE_SLIDER:
                        holdLayout.setVisibility(View.GONE);
                        styleLayout.setVisibility(View.GONE);
                        sliderStyleLayout.setVisibility(View.VISIBLE);
                        sliderStyleSpinner.setSelection(Utility.ArrayIndexOf(sliderStyleValues, btn.style));
                        keyLayout.setVisibility(View.VISIBLE);
                        key2Layout.setVisibility(View.VISIBLE);
                        key3Layout.setVisibility(View.VISIBLE);
                        keySpinner.setSelection(Utility.ArrayIndexOf(keyCodes, btn.key[0]));
                        key2Spinner.setSelection(Utility.ArrayIndexOf(keyCodes, btn.key[1]));
                        key3Spinner.setSelection(Utility.ArrayIndexOf(keyCodes, btn.key[2]));
                        break;
                    default:
                        break;
                }
            }
        };
        runnable.run();
        typeSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
           public void onItemSelected(AdapterView adapter, View view, int position, long id)
           {
               int type = typeValues[position];
               switch(type)
               {
                   case Q3EGlobals.TYPE_BUTTON:
                       holdLayout.setVisibility(View.VISIBLE);
                       styleLayout.setVisibility(View.VISIBLE);
                       keyLayout.setVisibility(View.VISIBLE);
                       key2Layout.setVisibility(View.GONE);
                       key3Layout.setVisibility(View.GONE);
                       sliderStyleLayout.setVisibility(View.GONE);
                       break;
                   case Q3EGlobals.TYPE_SLIDER:
                       holdLayout.setVisibility(View.GONE);
                       styleLayout.setVisibility(View.GONE);
                       sliderStyleLayout.setVisibility(View.VISIBLE);
                       keyLayout.setVisibility(View.VISIBLE);
                       key2Layout.setVisibility(View.VISIBLE);
                       key3Layout.setVisibility(View.VISIBLE);
                       break;
                   default:
                       break;
               }
           }
           public void onNothingSelected(AdapterView adapter)
           {

           }
        });
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(btn.name);
        builder.setView(view);
        builder.setPositiveButton(R.string.save, new AlertDialog.OnClickListener() {
            public void onClick(DialogInterface dialog, int button) {
                int type = typeValues[typeSpinner.getSelectedItemPosition()];
                btn.type = type;
                switch(type)
                {
                    case Q3EGlobals.TYPE_BUTTON:
                        btn.hold = holdCheckBox.isChecked();
                        btn.style = styleValues[styleSpinner.getSelectedItemPosition()];
                        btn.key[0] = keyCodes[keySpinner.getSelectedItemPosition()];
                        break;
                    case Q3EGlobals.TYPE_SLIDER:
                        btn.style = sliderStyleValues[sliderStyleSpinner.getSelectedItemPosition()];
                        btn.key[0] = keyCodes[keySpinner.getSelectedItemPosition()];
                        btn.key[1] = keyCodes[key2Spinner.getSelectedItemPosition()];
                        btn.key[2] = keyCodes[key3Spinner.getSelectedItemPosition()];
                        break;
                    default:
                        return;
                }
                btn.Update();
                SaveConfig();
            }
        });
        builder.setNegativeButton(R.string.cancel, null);
        builder.setNeutralButton(R.string.restore, null);
        AlertDialog dialog = builder.create();
        dialog.show();
        dialog.getButton(DialogInterface.BUTTON_NEUTRAL).setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                runnable.run();
            }
        });
    }

    private void OpenWeaponPanelKeysSetting()
    {
        final String[] Keys = {
            "1", "2", "3", "4", "5", "6", "7", "8", "9", "q", "0",
        };
        final StringBuilder sb = new StringBuilder();
        for(int i = 0; i < Keys.length; i++)
        {
            sb.append(Keys[i]);
            if(i < Keys.length - 1)
                sb.append(",");
        }
        final boolean[] states = new boolean[Keys.length];

        final SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        String curKeys = mPrefs.getString(Q3EPreference.WEAPON_PANEL_KEYS, sb.toString());
        if(null != curKeys && !curKeys.isEmpty())
        {
            List<String> keyList = Arrays.asList(curKeys.split(","));
            for(int i = 0; i < Keys.length; i++)
            {
                states[i] = keyList.contains(Keys[i]);
            }
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(R.string.weapon_panel_keys);
        builder.setMultiChoiceItems(Keys, states, null);
        builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int id)
                {
                    final ListView lst = ((AlertDialog)dialog).getListView();
                    SparseBooleanArray list = lst.getCheckedItemPositions();
                    String keyStr = "";
                    if(list.size() > 0)
                    {
                        Adapter adapter = lst.getAdapter();
                        List<Object> tsb = new ArrayList<>();
                        for(int i = 0; i < list.size(); i++)
                        {
                            int key = list.keyAt(i);
                            if(!list.get(key))
                                continue;
                            tsb.add(adapter.getItem(key));
                        }
                        keyStr = KStr.Join(tsb, ",");
                    }
                    mPrefs.edit().putString(Q3EPreference.WEAPON_PANEL_KEYS, keyStr).commit();
                }
            });
        builder.setNegativeButton(R.string.cancel, null);
        AlertDialog dialog = builder.create();
        dialog.show();
	}
}
