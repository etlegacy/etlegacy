package com.karin.idTech4Amm;

import android.app.Activity;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.sys.Constants;
import com.karin.idTech4Amm.sys.PreferenceKey;
import com.karin.idTech4Amm.sys.Theme;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KLogcat;
import com.etlegacy.app.R;

/**
 * logcat viewer
 */
public class LogcatActivity extends Activity
{
    private final ViewHolder V = new ViewHolder();
    private KLogcat m_logcat;
    private final KLogcat.KLogcatCallback m_callback = new KLogcat.KLogcatCallback() {
        @Override
        public void Output(String str)
        {
            runOnUiThread(new Runnable()
            {
                @Override
                public void run()
                {
                    V.logtext.append(str + "\n");
                    if(null != V.scrollCheckBox && V.scrollCheckBox.isChecked())
                        V.logscroll.smoothScrollTo(0, V.logtext.getHeight());
                }
            });
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Q3ELang.Locale(this);

        boolean o = PreferenceManager.getDefaultSharedPreferences(this).getBoolean(PreferenceKey.LAUNCHER_ORIENTATION, false);
        ContextUtility.SetScreenOrientation(this, o ? 0 : 1);

        Theme.SetTheme(this, false);
        setContentView(R.layout.logcat_page);

        m_logcat = new KLogcat();
        m_logcat.SetCommand("logcat | grep idTech4Amm");

        V.SetupUI();

        SetupUI();
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        Start();
    }

    @Override
    protected void onPause()
    {
        super.onPause();
        Stop();
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        Stop();
    }

    private void Start()
    {
        m_logcat.Start(m_callback);
        if(null != V.runBtn)
            V.runBtn.setTitle(R.string.stop);
    }

    private void Stop()
    {
        if(null != V.runBtn)
            V.runBtn.setTitle(R.string.start);
        if(null != m_logcat)
            m_logcat.Stop();
    }

    private void SetupUI()
    {
        //V.logtext.setTextColor(Theme.BlackColor(this));
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.logcat_menu, menu);
        V.SetupMenu(menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        int itemId = item.getItemId();
        if (itemId == R.id.logcat_menu_clear)
        {
            V.logtext.setText("");
        }
        else if (itemId == R.id.logcat_menu_dump)
        {
            String path = Q3EUtils.GetAppStoragePath(this, "/logcat");
            if(Q3EUtils.mkdir(path, true))
            {
                String filePath = path + "/" + Constants.CONST_APP_NAME + "_logcat.log";
                Q3EUtils.file_put_contents(filePath, V.logtext.getText().toString());
                Toast.makeText(LogcatActivity.this, "Save logcat to " + filePath, Toast.LENGTH_LONG).show();
            }
            else
                Toast.makeText(LogcatActivity.this, R.string.fail, Toast.LENGTH_LONG).show();
        }
        else if (itemId == R.id.logcat_menu_run)
        {
            if(null != m_logcat && m_logcat.IsRunning())
                Stop();
            else
                Start();
        }
        else if (itemId == R.id.logcat_menu_up)
        {
            V.logscroll.scrollTo(0, 0);
        }
        else if (itemId == R.id.logcat_menu_down)
        {
            V.logscroll.scrollTo(0, V.logtext.getHeight());
        }
        else if (itemId == R.id.logcat_menu_scroll)
        {
            V.scrollCheckBox.setChecked(!V.scrollCheckBox.isChecked());
            boolean checked = V.scrollCheckBox.isChecked();
            if(checked)
                V.logscroll.scrollTo(0, V.logtext.getHeight());
            V.scrollCheckBox.setTitle(checked ? R.string.pause : R.string.scroll);
        }
        return super.onOptionsItemSelected(item);
    }

    private class ViewHolder
    {
        private TextView logtext;
        private ScrollView logscroll;
        private MenuItem runBtn;
        private MenuItem scrollCheckBox;

        public void SetupUI()
        {
            logtext = findViewById(R.id.logtext);
            logscroll = findViewById(R.id.logscroll);
        }

        public void SetupMenu(Menu menu)
        {
            runBtn = menu.findItem(R.id.logcat_menu_run);
            runBtn.setTitle(R.string.stop);
            scrollCheckBox = menu.findItem(R.id.logcat_menu_scroll);
            scrollCheckBox.setTitle(R.string.scroll);
        }
    }
}
