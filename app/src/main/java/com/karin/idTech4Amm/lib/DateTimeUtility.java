package com.karin.idTech4Amm.lib;

import java.text.SimpleDateFormat;
import java.util.Date;

public final class DateTimeUtility
{
    public static String Format(Date date, String format)
    {
        SimpleDateFormat f = new SimpleDateFormat(format);
        return f.format(date);
    }

    public static String Format(long ts, String format)
    {
        return Format(new Date(ts), format);
    }

    private DateTimeUtility() {}
}
