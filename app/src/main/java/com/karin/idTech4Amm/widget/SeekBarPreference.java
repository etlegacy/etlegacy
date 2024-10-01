package com.karin.idTech4Amm.widget;

import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.content.Context;
import android.view.View;
import android.widget.SeekBar;
import java.util.Map;
import java.util.HashMap;
import android.widget.TextView;
import android.graphics.Color;
import android.content.res.TypedArray;
import android.annotation.SuppressLint;
import android.os.Build;

import com.etlegacy.app.R;

/**
 * SeekBar preference widget
 */
public class SeekBarPreference extends DialogPreference
{
    private Attr m_initValues = null;
    private final ViewHolder V = new ViewHolder();

    @SuppressLint("NewApi")
    public SeekBarPreference(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
    {
        super(context, attrs, defStyleAttr, defStyleRes);
        CreateUI(attrs);
    }

    public SeekBarPreference(Context context, AttributeSet attrs, int defStyleAttr)
    {
        super(context, attrs, defStyleAttr);
        CreateUI(attrs);
    }

    public SeekBarPreference(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        CreateUI(attrs);
    }

    public SeekBarPreference(Context context)
    {
        this(context, null);
    }

    private void CreateUI(AttributeSet attrs)
    {
        setDialogLayoutResource(R.layout.seek_bar_dialog_preference);
        if(attrs != null)
        {
            TypedArray ta = getContext().obtainStyledAttributes(attrs, R.styleable.SeekBarDialogPreference);
            m_initValues = new Attr();
            m_initValues.min = ta.getInt(R.styleable.SeekBarDialogPreference_min, 0);
            m_initValues.max = ta.getInt(R.styleable.SeekBarDialogPreference_max, 100);
            String suffix = ta.getString(R.styleable.SeekBarDialogPreference_suffix);
            if(null == suffix)
                suffix = "";
            m_initValues.suffix = suffix;
            m_initValues.editable = ta.getBoolean(R.styleable.SeekBarDialogPreference_editable, true);
            ta.recycle();
        }
    }

    private void SetupUI(View view)
    {
        V.Setup(view);
        if(m_initValues != null)
        {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                V.seek_bar.setMin(m_initValues.min);
            }
            V.seek_bar.setMax(m_initValues.max);
            V.suffix.setText(m_initValues.suffix);
            V.seek_bar.setEnabled(m_initValues.editable);
        }
        V.seek_bar.setProgress(getPersistedInt(0));
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            V.min.setText("" + V.seek_bar.getMin());
        }
        V.max.setText("" + V.seek_bar.getMax());
        V.progress.setText("" + V.seek_bar.getProgress());
        V.progress.setTextColor(Color.GRAY);
        V.seek_bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
                {
                    V.progress.setText("" + progress);
                }
                public void onStartTrackingTouch(SeekBar seekBar)
                {
                    V.progress.setTextColor(Color.RED);
                }
                public void onStopTrackingTouch(SeekBar seekBar)
                {
                    V.progress.setTextColor(Color.GRAY);
                }
        });
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);
        if(positiveResult)
            persistInt(V.seek_bar.getProgress());
    }

    @Override
    protected View onCreateDialogView()
    {
        View view = super.onCreateDialogView();
        SetupUI(view);
        return view;
    }

    private class ViewHolder
    {
        private SeekBar seek_bar;
        private TextView min;
        private TextView max;
        private TextView progress;
        private TextView suffix;

        public void Setup(View view)
        {
            seek_bar = view.findViewById(R.id.seek_bar_dialog_preference_layout_seekbar);
            min = view.findViewById(R.id.seek_bar_dialog_preference_layout_min);
            max = view.findViewById(R.id.seek_bar_dialog_preference_layout_max);
            progress = view.findViewById(R.id.seek_bar_dialog_preference_layout_progress);
            suffix = view.findViewById(R.id.seek_bar_dialog_preference_layout_suffix);
        }
    }

    private static class Attr
    {
        public int min = 0;
        public int max = 100;
        public String suffix = "";
        public boolean editable = true;
    }
}
