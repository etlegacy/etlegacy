package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.TextView;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.sys.Constants;
import com.karin.idTech4Amm.ui.ArrayAdapter_base;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;

import java.util.ArrayList;
import java.util.List;

public final class TranslatorsFunc extends GameLauncherFunc
{
    public TranslatorsFunc(GameLauncher gameLauncher)
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
        builder.setTitle(Q3ELang.tr(m_gameLauncher, R.string.translators) + "(" + Q3ELang.tr(m_gameLauncher, R.string.sorting_by_add_time) + ")");
        ListView view = new ListView(m_gameLauncher);
        view.setAdapter(new TranslatorAdapter(m_gameLauncher));
        view.setDivider(null);

        builder.setView(view);
        builder.setPositiveButton(R.string.ok, null)
                .setNeutralButton(R.string.about, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                ContextUtility.OpenMessageDialog(m_gameLauncher, m_gameLauncher.getString(R.string.note), TextHelper.GetDialogMessage(
                        "Translations are supplied by volunteers." + TextHelper.GetDialogMessageEndl()
                        + "Thanks for all everyone working." + TextHelper.GetDialogMessageEndl()
                        + "If has some incorrect, ambiguous and sensitive words, you can contact the translator by clicking name."
                ));
            }
        });
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    private static class TranslatorAdapter extends ArrayAdapter_base<Translator>
    {
        public TranslatorAdapter(Context context)
        {
            super(context, R.layout.translators_list_delegate);

            List<Translator> m_list = new ArrayList<>();

            Translator tr;

            tr = new Translator();
/*        tr.lang = "English";
        tr.author = "n0n3m4";
        m_list.add(tr);*/

            tr = new Translator();
            tr.lang = "中文";
            tr.author = "Karin Zhao";
            tr.url = Constants.CONST_EMAIL;
            m_list.add(tr);

            tr = new Translator();
            tr.lang = "Русский";
            tr.author = "ALord7";
            tr.group = "4pda";
            tr.url = "https://4pda.ru/forum/index.php?showuser=5043340";
            m_list.add(tr);

            SetData(m_list);
        }

        @Override
        public View GenerateView(int position, View view, ViewGroup parent, Translator data)
        {
            TextView textView = view.findViewById(R.id.translators_list_delegate_lang);
            textView.setText(data.lang);
            textView = view.findViewById(R.id.translators_list_delegate_author);
            textView.setText(data.Name());
            textView.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    data.Open(getContext());
                }
            });

            return view;
        }
    }

    private static class Translator
    {
        public String lang;
        public String author;
        public String group;
        public String url;

        public String Name()
        {
            String str = author;
            if(null != group)
                str += "@" + group;
            return str;
        }

        public void Open(Context context)
        {
            if(null == url || url.isEmpty())
                return;
            try
            {
                if(url.startsWith("http"))
                    ContextUtility.OpenUrlExternally(context, url);
                else
                {
                    ContextUtility.OpenUrlExternally(context, "mailto:" + url);
                }
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
        }
    }
}
