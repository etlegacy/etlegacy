package com.n0n3m4.q3e.onscreen;

public class Q3EButtonGeometry
{
    public int x, y, width_or_radius, alpha;

    Q3EButtonGeometry()
    {
    }

    public Q3EButtonGeometry(Number x, Number y, Number r_or_w, Number a)
    {
        this.Set(x, y, r_or_w, a);
    }

    public void Set(Number x, Number y, Number r_or_w, Number a)
    {
        this.Set(x, y, r_or_w);
        this.alpha = a.intValue();
    }

    public void Set(Number x, Number y, Number r_or_w)
    {
        this.x = x.intValue();
        this.y = y.intValue();
        this.width_or_radius = r_or_w.intValue();
    }

    @Override
    public String toString()
    {
        return x + " " + y + " " + width_or_radius + " " + alpha;
    }

    public static String[] ToStrings(Q3EButtonGeometry[] layouts)
    {
        String[] defaults_table = new String[layouts.length];
        for (int i = 0; i < layouts.length; i++)
        {
            defaults_table[i] = layouts[i].toString();
        }
        return defaults_table;
    }

    public static Q3EButtonGeometry[] Alloc(int size)
    {
        Q3EButtonGeometry[] layouts = new Q3EButtonGeometry[size];
        for (int i = 0; i < layouts.length; i++)
        {
            layouts[i] = new Q3EButtonGeometry();
        }
        return layouts;
    }
}
