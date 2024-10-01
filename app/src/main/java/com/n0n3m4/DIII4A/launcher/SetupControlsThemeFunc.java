package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.sys.Constants;
import com.karin.idTech4Amm.ui.ArrayAdapter_base;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KFDManager;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;

public final class SetupControlsThemeFunc extends GameLauncherFunc
{
    public SetupControlsThemeFunc(GameLauncher gameLauncher)
    {
        super(gameLauncher);
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        run();
    }

    public void run()
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.controls_theme);
        View widget = m_gameLauncher.getLayoutInflater().inflate(R.layout.controls_theme_dialog, null, false);
        LinkedHashMap<String, String> schemes = Q3EUtils.GetControlsThemes(m_gameLauncher);

        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher);
        String type = preferences.getString(Q3EPreference.CONTROLS_THEME, "");
        if(null == type)
            type = "";
        String[] theme = { type };
        ArrayList<String> types = new ArrayList<>(schemes.keySet());

        Spinner spinner = widget.findViewById(R.id.controls_theme_spinner);
        final ArrayAdapter<String> typeAdapter = new ArrayAdapter<>(m_gameLauncher, android.R.layout.simple_spinner_dropdown_item, new ArrayList<>(schemes.values()));
        typeAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(typeAdapter);
        spinner.setSelection(types.indexOf(theme[0]));
        ListView list = widget.findViewById(R.id.controls_theme_list);
        ControlsThemeAdapter adapter = new ControlsThemeAdapter(widget.getContext());
        list.setAdapter(adapter);
        adapter.Update(theme[0]);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id)
            {
                theme[0] = types.get(position);
                adapter.Update(theme[0]);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent)
            {
            }
        });

        builder.setView(widget);
        builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                preferences.edit().putString(Q3EPreference.CONTROLS_THEME, theme[0]).commit();
            }
        })
            .setNeutralButton(R.string.tips, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    ShowTips();
                }
            })
                .setNegativeButton(R.string.cancel, null);
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    private void ShowTips()
    {
        final String endl = TextHelper.GetDialogMessageEndl();
        KFDManager manager = KFDManager.Instance(m_gameLauncher);
        StringBuilder sb = new StringBuilder();
        String[] sps = manager.GetSearchPathFolders();
        int i = 0;
        for (String sp : sps)
        {
            sb.append(" ").append(++i).append(". ").append(sp).append(endl);
            sb.append(" ").append(++i).append(". ").append(sp).append("/*.zipak").append(endl);
        }
        final String Tips = "If choosing `External`, put button image files to `&lt;EXTERNAL SEARCH PATH&gt;` as same file name, will instead of apk internal image files."
                + endl
                + "Or putting button image files as a folder with your custom name in `&lt;EXTERNAL SEARCH PATH&gt;/controls_theme/`, the Theme chooser will show the folder name, and select the folder name instead of apk internal image files."
                + endl + endl
                + "&lt;CURRENT ALL SEARCH PATH&gt; (.zipak is zip archive file): " + endl
                + sb
                ;
        ContextUtility.OpenMessageDialog(m_gameLauncher, Q3ELang.tr(m_gameLauncher, R.string.tips), TextHelper.GetDialogMessage(Tips));
    }

    private static class ControlsThemeAdapter extends ArrayAdapter_base<ControlsTheme>
    {
        private final List<ControlsTheme> m_list = new ArrayList<>();

        public ControlsThemeAdapter(Context context)
        {
            super(context, R.layout.controls_theme_list_delegate);

            Q3EInterface q3ei = Q3EUtils.q3ei;
            // joystick
            String str = Q3EUtils.q3ei.texture_table[Q3EGlobals.UI_JOYSTICK];
            if(null != str && !str.isEmpty())
            {
                String[] split = str.split(";");
                String name = split[0];
                ControlsTheme theme = new ControlsTheme();
                theme.name = Q3ELang.tr(getContext(), R.string.joystick_background);
                theme.path = name;
                m_list.add(theme);
                name = split[1];
                theme = new ControlsTheme();
                theme.name = Q3ELang.tr(getContext(), R.string.joystick_center);
                theme.path = name;
                m_list.add(theme);
            }

            for(int i = 0; i < Q3EGlobals.UI_SIZE; i++)
            {
                int type = q3ei.type_table[i];
                if(type == Q3EGlobals.TYPE_JOYSTICK || type == Q3EGlobals.TYPE_DISC)
                    continue;
                String name = Q3EUtils.q3ei.texture_table[i];
                if(type == Q3EGlobals.TYPE_SLIDER)
                {
                    ControlsTheme theme = new ControlsTheme();
                    theme.name = Q3EGlobals.CONTROLS_NAMES[i];
                    if(null != name && !name.isEmpty())
                    {
                        String[] split = name.split(";");
                        theme.path = split[0];
                        m_list.add(theme);
                        if(split.length >= 4)
                        {
                            for(int m = 1; m <= 3; m++)
                            {
                                theme = new ControlsTheme();
                                theme.name = Q3EGlobals.CONTROLS_NAMES[i] + " " + m;
                                theme.path = split[m];
                                m_list.add(theme);
                            }
                        }
                    }
                    else
                        m_list.add(theme);
                }
                else
                {
                    ControlsTheme theme = new ControlsTheme();
                    theme.name = Q3EGlobals.CONTROLS_NAMES[i];
                    theme.path = name;
                    m_list.add(theme);
                }
            }

            // weapon panel
            str = Q3EUtils.q3ei.texture_table[Q3EGlobals.UI_WEAPON_PANEL];
            ControlsTheme theme = new ControlsTheme();
            theme.name = Q3ELang.tr(getContext(), R.string.weapon_panel);
            if(null != str && !str.isEmpty())
            {
                String[] split = str.split(";");
                theme.path = split[0];
            }
            m_list.add(theme);
            SetData(m_list);
        }

        @Override
        public View GenerateView(int position, View view, ViewGroup parent, ControlsTheme data)
        {
            ImageView image = view.findViewById(R.id.controls_theme_list_delegate_image);

            if(null != data.texture)
                image.setImageBitmap(data.texture);
            else
                image.setImageDrawable(new ColorDrawable(Color.BLACK));

            TextView textView = view.findViewById(R.id.controls_theme_list_delegate_name);
            textView.setText(data.name);
            textView = view.findViewById(R.id.controls_theme_list_delegate_path);
            textView.setText(data.path);
            textView = view.findViewById(R.id.controls_theme_list_delegate_size);
            textView.setText(data.GetTextureSize());

            return view;
        }

        public void Update(String themeName)
        {
            for (ControlsTheme theme : m_list)
            {
                theme.Release();
                theme.Load(getContext(), themeName);
            }
            notifyDataSetChanged();
        }
    }

    private static class ControlsTheme
    {
        public String name;
        public String path;
        public Bitmap texture;

        public void Release()
        {
            if(null != texture)
            {
                texture.recycle();
                texture = null;
            }
        }

        public void Load(Context context, String type)
        {
            texture = Q3EUtils.LoadControlBitmap(context, path, type);
        }

        public String GetTextureSize()
        {
            return null != texture ? String.format("%d x %d", texture.getWidth(), texture.getHeight()) : "";
        }
    }
}
