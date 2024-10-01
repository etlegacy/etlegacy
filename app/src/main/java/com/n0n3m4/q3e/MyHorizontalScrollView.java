package com.n0n3m4.q3e;

//import org.apache.http.impl.entity.LaxContentLengthStrategy;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.HorizontalScrollView;

public class MyHorizontalScrollView extends HorizontalScrollView {
	public static int maxscrollx=0;
	public static int minscrollx=0;
	public static int deltascrollx=0;
    public MyHorizontalScrollView(Context context)
    {
        super(context);
    }
    public MyHorizontalScrollView(Context context, AttributeSet attrs)
    {
        super(context, attrs);
    }
    public MyHorizontalScrollView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void onScrollChanged(int l, int t, int oldl, int oldt) {    	
        if ((maxscrollx!=0)&&(l>maxscrollx))
        {
        	l-=deltascrollx;
        	scrollTo(l, t);
        }
        else
        if ((minscrollx!=0)&&(l<minscrollx))
        {
          	l+=deltascrollx;
          	scrollTo(l, t);
        }
        else
        super.onScrollChanged(l, t, oldl, oldt);
    }

}
