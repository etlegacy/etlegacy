package com.n0n3m4.q3e.karin;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.ListAdapter;

public class KHorizontalListView extends HorizontalScrollView {
	private LinearLayout m_mainLayout;
    private GridView m_gridView;
    private int m_horizontalSpacing = 2;
    private int m_columnWidth = 256;

    public KHorizontalListView(Context context)
    {
        super(context);
        Setup(null);
    }
    public KHorizontalListView(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        Setup(attrs);
    }
    public KHorizontalListView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        Setup(attrs);
    }

    void Setup(AttributeSet attrs)
    {
        Context context = getContext();
        m_gridView = new GridView(context);
        m_gridView.setStretchMode(GridView.STRETCH_SPACING);
        m_gridView.setHorizontalSpacing(m_horizontalSpacing);
        m_gridView.setColumnWidth(m_columnWidth);

        m_mainLayout = new LinearLayout(context);
        m_mainLayout.setVerticalGravity(Gravity.CENTER_VERTICAL);

        ViewGroup.LayoutParams gridParams = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT);
        m_mainLayout.addView(m_gridView, gridParams);

        ViewGroup.LayoutParams params = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT);
        addView(m_mainLayout, params);
    }

    public void SetAdapter(ListAdapter adapter)
    {
        m_gridView.setAdapter(adapter);
        UpdateLayout();
    }

    public void UpdateLayout()
    {
        ListAdapter adapter = m_gridView.getAdapter();
        int count = adapter.getCount();
        m_gridView.setNumColumns(count);
        ViewGroup.LayoutParams layoutParams = m_gridView.getLayoutParams();
        layoutParams.width = count * m_columnWidth + m_horizontalSpacing * (Math.max(0, count - 1));
        m_mainLayout.updateViewLayout(m_gridView, layoutParams);
    }

    public void SetHorizontalSpacing(int spacing)
    {
        if(m_horizontalSpacing != spacing)
        {
            m_horizontalSpacing = spacing;
            m_gridView.setHorizontalSpacing(m_horizontalSpacing);
            UpdateLayout();
        }
    }

    public void SetColumnWidth(int width)
    {
        if(m_columnWidth != width)
        {
            m_columnWidth = width;
            m_gridView.setColumnWidth(m_columnWidth);
            UpdateLayout();
        }
    }
}
