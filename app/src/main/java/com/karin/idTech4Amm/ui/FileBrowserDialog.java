package com.karin.idTech4Amm.ui;

import android.content.Context;
import android.os.Bundle;

import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;

import android.app.AlertDialog;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.misc.FileBrowser;
import com.n0n3m4.q3e.Q3ELang;

/**
 * Simple file chooser
 */
public class FileBrowserDialog extends AlertDialog {
    private static final String TAG = "FileBrowserDialog";

    private FileViewAdapter m_adapter;
    private ListView m_listView;
    private String m_title;
    private FileBrowserCallback m_callback;
    private final FileViewAdapter.FileAdapterListener m_listener = new FileViewAdapter.FileAdapterListener()
    {
        @Override
        public void OnPathChanged(String path)
        {
            FileBrowserDialog.this.setTitle(m_title + ": \n" + path);
            m_listView.setSelection(0);
        }

        @Override
        public void OnFileSelected(String file, String path)
        {

        }

        @Override
        public void OnGrantPermission(String path)
        {
            if(null != m_callback)
                m_callback.Check(path);
        }

    };

    public FileBrowserDialog(Context context)
    {
        super(context);
        m_title = Q3ELang.tr(context, R.string.file_chooser);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //SetupUI();
    }

    public void SetupUI(String title)
    {
        SetupUI(title, null);
    }

    public void SetupUI(String title, String path)
    {
        Context context = getContext();

        m_title = title;

        setTitle(m_title);
        m_listView = new ListView(context);
        setView(m_listView);

        m_adapter = new FileViewAdapter(context, null);
        m_listView.setAdapter(m_adapter);
        m_listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                    FileBrowserDialog.this.Open(position);
                }
            });
        m_adapter.SetListener(m_listener);
        if(null != path)
            SetPath(path);
    }

    private void Open(int position)
    {
        if(m_adapter.IsDirectory(position))
        {
            String path = m_adapter.Get(position);
            SetPath(path);
        }
    }

    public void SetPath(String path)
    {
        m_adapter.OpenPath(path);
    }

    public String Path()
    {
        return m_adapter.Path();
    }

    public void SetCallback(FileBrowserCallback c)
    {
        m_callback = c;
    }

    public FileBrowser GetFileBrowser()
    {
        return m_adapter.GetFileBrowser();
    }

    public interface FileBrowserCallback
    {
        public void Check(String path);
    }
}

