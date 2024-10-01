package com.karin.idTech4Amm.widget;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.etlegacy.app.R;

import java.util.HashMap;
import java.util.Map;

/**
 * Divider widget
 */
public class Divider extends LinearLayout
{
    private Attr m_initValues = null;
    private final ViewHolder V = new ViewHolder();

    @SuppressLint("NewApi")
    public Divider(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
    {
        super(context, attrs, defStyleAttr, defStyleRes);
        CreateUI(attrs);
    }

    public Divider(Context context, AttributeSet attrs, int defStyleAttr)
    {
        super(context, attrs, defStyleAttr);
        CreateUI(attrs);
    }

    public Divider(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        CreateUI(attrs);
    }

    public Divider(Context context)
    {
        this(context, null);
    }

    private void CreateUI(AttributeSet attrs)
    {
        View inflate = LayoutInflater.from(getContext()).inflate(R.layout.divider, this, false);
        addView(inflate);
        if(attrs != null)
        {
            TypedArray ta = getContext().obtainStyledAttributes(attrs, R.styleable.Divider);
            m_initValues = new Attr();
            m_initValues.lineColor = ta.getColor(R.styleable.Divider_lineColor, Color.argb(255, 0xCC, 0xCC, 0xCC));
            String label = ta.getString(R.styleable.Divider_label);
            if(null == label)
                label = "";
            m_initValues.label = label;
            ta.recycle();
        }
        SetupUI(inflate);
    }

    private void SetupUI(View view)
    {
        V.Setup(view);
        if(m_initValues != null)
        {
            V.text_label.setText(m_initValues.label);
            V.left_line.setBackgroundColor(m_initValues.lineColor);
            V.right_line.setBackgroundColor(m_initValues.lineColor);
        }
    }

    private class ViewHolder
    {
        private TextView text_label;
        private View left_line;
        private View right_line;

        public void Setup(View view)
        {
            text_label = view.findViewById(R.id.divider_text_label);
            left_line = view.findViewById(R.id.divider_left_line);
            right_line = view.findViewById(R.id.divider_right_line);
        }
    }

    private static class Attr
    {
        public int lineColor = Color.argb(255, 0xCC, 0xCC, 0xCC);
        public String label = "";
    }
}
