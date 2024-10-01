package com.karin.idTech4Amm.ui;

import android.content.Context;
import android.graphics.Color;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.karin.idTech4Amm.misc.FileBrowser;
import com.karin.idTech4Amm.sys.Theme;

public class FileViewAdapter extends ArrayAdapter_base<FileBrowser.FileModel>
{
    private static final String TAG = "FileViewAdapter";

    private String m_path;
    private String m_file;

    private FileAdapterListener m_listener;

    private final FileBrowser m_fileBrowser;

    public FileViewAdapter(Context context, String path)
    {
        super(context, android.R.layout.select_dialog_item);
        m_fileBrowser = new FileBrowser(context);
        m_fileBrowser.SetListener(new FileBrowser.Listener()
        {
            @Override
            public void OnPathCannotAccess(String path)
            {
                if(null != m_listener)
                    m_listener.OnGrantPermission(path);
            }
        });
        if(null != path && !path.isEmpty())
            OpenPath(path);
    }

    public View GenerateView(int position, View view, ViewGroup parent, FileBrowser.FileModel data)
    {
        TextView textView;

        textView = (TextView)view;
        textView.setText(data.name);
        textView.setTextColor(data.IsDirectory() ? Theme.BlackColor(getContext()) : Color.GRAY);
        return view;
    }

    public void Open(int index)
    {
        FileBrowser.FileModel item;

        item = m_fileBrowser.GetFileModel(index);
        if(item == null)
            return;
        if(item.type != FileBrowser.FileModel.ID_FILE_TYPE_DIRECTORY) // TODO: symbol file
        {
            Log.d(TAG, "Open file: " + item.path);
            SetFile(item.path);
            // open
            return;
        }

        OpenPath(item.path);
    }

    public String Get(int index)
    {
        FileBrowser.FileModel item;

        item = m_fileBrowser.GetFileModel(index);
        if(item == null)
            return null;
        return item.path;
    }

    public boolean IsDirectory(int index)
    {
        FileBrowser.FileModel item;

        item = m_fileBrowser.GetFileModel(index);
        if(item == null)
            return false;
        return item.type == FileBrowser.FileModel.ID_FILE_TYPE_DIRECTORY;
    }

    public void OpenPath(String path)
    {
        boolean res;

        Log.d(TAG, "Open directory: " + path);
        res = m_fileBrowser.SetCurrentPath(path);

        if(true || res) // always update
        {
            SetData(m_fileBrowser.FileList());
            SetPath(m_fileBrowser.CurrentPath());
            SetFile(null);
        }
    }

    private void SetPath(String path)
    {
        m_path = path;
        if(m_listener != null)
            m_listener.OnPathChanged(m_path);
    }

    private void SetFile(String file)
    {
        m_file = file;
        if(m_listener != null)
            m_listener.OnFileSelected(m_file, m_path);
    }

    public String Path()
    {
        return m_path;
    }

    public String File()
    {
        return m_file;
    }

    public FileBrowser GetFileBrowser()
    {
        return m_fileBrowser;
    }

    public interface FileAdapterListener
    {
        public void OnPathChanged(String path);
        public void OnFileSelected(String file, String path);
        public void OnGrantPermission(String path);
    }

    public void SetListener(FileAdapterListener l)
    {
        m_listener = l;
    }
}
