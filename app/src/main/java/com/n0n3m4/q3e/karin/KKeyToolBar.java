package com.n0n3m4.q3e.karin;

import android.content.Context;
import android.content.res.Resources;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;
import com.etlegacy.app.R;

import java.util.ArrayList;
import java.util.List;

public class KKeyToolBar extends LinearLayout {
    private int toolbar_key_text_released;
    private int toolbar_key_text_pressed;
    private int toolbar_key_bg_released;
    private int toolbar_key_bg_pressed;
    private KeyListAdapter m_keyListAdapter;

    public KKeyToolBar(Context context)
    {
        super(context);
        Setup();
    }
    public KKeyToolBar(Context context, AttributeSet attrs)
    {

        super(context, attrs);
        Setup();
    }
    public KKeyToolBar(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        Setup();
    }

    private final Runnable m_updateToolBar = new Runnable() {
        @Override
        public void run() {
            m_keyListAdapter.notifyDataSetChanged();
        }
    };

    private final View.OnTouchListener m_keyListener = new OnTouchListener() {
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            Object obj = v.getTag();
            if(null == obj)
                return false;
            Key key = (Key)obj;
            boolean down = false;
            switch (event.getActionMasked())
            {
                case MotionEvent.ACTION_DOWN:
                case MotionEvent.ACTION_POINTER_DOWN:
                    key.state = Key.STATE_PRESSED;
                    down = true;
                    break;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_POINTER_UP:
                case MotionEvent.ACTION_CANCEL:
                    key.state = Key.STATE_RELEASED;
                    break;
                default:
                    return false;
            }
            post(m_updateToolBar);
            Q3EUtils.q3ei.callbackObj.sendKeyEvent(down, key.keyCode, 0);
            return true;
        }
    };

    void Setup()
    {
        Resources resources = getResources();
        toolbar_key_text_released = resources.getColor(R.color.toolbar_key_text_released);
        toolbar_key_bg_released = resources.getColor(R.color.toolbar_key_bg_released);
        toolbar_key_text_pressed = resources.getColor(R.color.toolbar_key_text_pressed);
        toolbar_key_bg_pressed = resources.getColor(R.color.toolbar_key_bg_pressed);

        setBackgroundColor(resources.getColor(R.color.toolbar_background));
        int[] keys = getResources().getIntArray(R.array.key_toolbar_codes);
        String[] values = getResources().getStringArray(R.array.key_toolbar_names);
        List<Key> list = new ArrayList<>();
        Key key;

        for (int i = 0; i < values.length; i++) {
            key = new Key();
            key.name = values[i];
            key.keyCode = Q3EKeyCodes.GetRealKeyCode(keys[i]);
            list.add(key);
        }

        int h = resources.getDimensionPixelSize(R.dimen.toolbarHeight);
        int m = resources.getDimensionPixelSize(R.dimen.toolFunctionMargin);
        int w = resources.getDimensionPixelSize(R.dimen.toolButtonWidth);
        int s = resources.getDimensionPixelSize(R.dimen.toolButtonSpacing);
        Context context = getContext();

        KHorizontalListView m_toolbar = new KHorizontalListView(context);
        m_keyListAdapter = new KeyListAdapter(R.layout.tool_button);
        m_keyListAdapter.SetData(list);
        m_toolbar.SetAdapter(m_keyListAdapter);
        m_toolbar.SetColumnWidth(w);
        m_toolbar.SetHorizontalSpacing(s);

        LinearLayout.LayoutParams layoutParms = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1);
        addView(m_toolbar, layoutParms);

        ImageView m_movableView = new ImageView(context);
        m_movableView.setImageDrawable(resources.getDrawable(R.drawable.icon_m_up_down));
        layoutParms = new LinearLayout.LayoutParams(h, ViewGroup.LayoutParams.MATCH_PARENT);
        layoutParms.setMargins(m, m, m, m);
        addView(m_movableView, layoutParms);

        ImageView m_closeView = new ImageView(context);
        m_closeView.setImageDrawable(getResources().getDrawable(R.drawable.icon_m_close));
        layoutParms = new LinearLayout.LayoutParams(h, ViewGroup.LayoutParams.MATCH_PARENT);
        layoutParms.setMargins(m, m, m, m);
        addView(m_closeView, layoutParms);

        setFocusable(false);
        setFocusableInTouchMode(false);
        m_movableView.setFocusable(false);
        m_movableView.setFocusableInTouchMode(false);
        m_closeView.setFocusable(false);
        m_closeView.setFocusableInTouchMode(false);
        m_toolbar.setFocusable(false);
        m_toolbar.setFocusableInTouchMode(false);

        m_movableView.setOnTouchListener(m_onTouchEvent);
        m_closeView.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                Q3EUtils.ToggleToolbar(false);
            }
        });
    }

    private static class Key
    {
        public static final int STATE_RELEASED = 0;
        public static final int STATE_PRESSED = 1;
        public static final int STATE_TOGGLED = 2;

        public String name;
        public int keyCode;
        public int state = STATE_RELEASED;
    }

    private class KeyListAdapter extends BaseAdapter {
        public int m_resource = 0;
        public final List<Key> list = new ArrayList<>();

        public KeyListAdapter(int resource)
        {
            m_resource = resource;
        }

        @Override
        public int getCount() {
            return list.size();
        }

        @Override
        public Object getItem(int position) {
            return list.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent)
        {
            Key obj = (Key) getItem(position);
            View view;

            if(convertView == null)
                view = LayoutInflater.from(parent.getContext()).inflate(m_resource, parent, false);
            else
                view = convertView;
            return GenerateView(position, view, parent, obj);
        }

        private void SetData(List<Key> data)
        {
            list.clear();
            if(null != data && !data.isEmpty())
                list.addAll(data);
            notifyDataSetChanged();
        }

        private View GenerateView(int position, View view, ViewGroup parent, Key data)
        {
            TextView textView = view.findViewById(R.id.tool_button_label);

            textView.setText(data.name);
            switch(data.state)
            {
                case Key.STATE_RELEASED:
                    textView.setTextColor(toolbar_key_text_released);
                    view.setBackgroundColor(toolbar_key_bg_released);
                    break;
                /*case Key.STATE_TOGGLED:
                    break;*/
                case Key.STATE_PRESSED:
                default:
                    textView.setTextColor(toolbar_key_text_pressed);
                    view.setBackgroundColor(toolbar_key_bg_pressed);
                    break;
            }
            view.setTag(data);
            view.setOnTouchListener(m_keyListener);
            view.setFocusable(false);
            view.setFocusableInTouchMode(false);
            return view;
        }
    }

    private int m_lastY;
    private boolean m_pressed = false;
    private final View.OnTouchListener m_onTouchEvent = new OnTouchListener() {
        @Override
        public boolean onTouch(View v, MotionEvent ev) {
            int y = (int)ev.getRawY();
            //Log.e("TAGID_TAG", String.format("%d %d|%d", x, y, ev.getAction()));
            switch (ev.getAction())
            {
                case MotionEvent.ACTION_DOWN:
                    if(!m_pressed)
                    {
                        m_pressed = true;
                        m_lastY = y;
                        return true;
                    }
                    break;
                case MotionEvent.ACTION_MOVE:
                    if(m_pressed)
                    {
                        int lastDeltaY = y - m_lastY;
                        if(lastDeltaY != 0)
                        {
                            int cury = (int) getY();
                            int posy = cury + lastDeltaY;
                            if(posy < 0)
                                posy = 0;
                            if(cury != posy)
                            {
                                setY(posy);
                                getParent().requestLayout();
                                Q3EPreference.SetStringFromInt(getContext(), Q3EPreference.pref_harm_function_key_toolbar_y, posy);
                            }
                        }
                        m_lastY = y;
                        return true;
                    }
                    break;
                case MotionEvent.ACTION_UP:
                    if(m_pressed)
                    {
                        Reset();
                        return true;
                    }
                    break;
                case MotionEvent.ACTION_CANCEL:
                    Reset();
                    return true;
                default:
                    break;
            }
            return false;
        }
    };

    private void Reset()
    {
        m_lastY = 0;
        m_pressed = false;
    }
}
