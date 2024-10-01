package com.n0n3m4.q3e.karin;

import com.n0n3m4.q3e.Q3EGlobals;

/**
 * idTech command line utility
 * prop: +/- set name value
 * param: +/-name value
 * bool: 1=true, 0=false
 * +: idTech2 3 4 TDM BFG
 * -: QuakeTech
 */
public class KidTechCommand
{
    public static final char ARG_PREFIX_IDTECH = '+';
    public static final char ARG_PREFIX_QUAKETECH = '-';

    private String m_cmd = Q3EGlobals.GAME_EXECUABLE;
    private char m_argPrefix = ARG_PREFIX_IDTECH;
    private static String PreCmd(char PLUS, String cmd)
    {
        if(null == cmd)
        {
            return "";
        }
        if(cmd.startsWith("" + PLUS))
            cmd = " " + cmd;
        return cmd;
    }

    private static String PostCmd(String cmd)
    {
        if(null == cmd)
        {
            return "";
        }
        int i = 0;
        int length = cmd.length();
        while(i < length)
        {
            if(!Character.isSpaceChar(cmd.charAt(i)))
                break;
            i++;
        }
        return i > 0 ? (i < length ? cmd.substring(i) : "") : cmd;
    }

    public static String btostr(boolean b)
    {
        return b ? "1" : "0";
    }

    public static boolean strtob(String str)
    {
        return "1".equals(str);
    }

    public static String SetProp(char PLUS, String str, String name, Object val)
    {
        str = PreCmd(PLUS, str);
        String nname = " " + PLUS + "set " + name;
        String insertCmd = val.toString().trim();
        int index = str.indexOf(nname);
        if (index != -1)
        {
            int start = index + nname.length();
            if(start == str.length()) // at last
            {
                str = str + " " + insertCmd;
            }
            else
            {
                nname += " ";
                start = str.indexOf(nname);
                if(start != -1)
                {
                    start += nname.length() - 1;
                    int end = str.indexOf(" " + PLUS, start);
                    if (end != -1)
                        str = str.substring(0, start) + " " + insertCmd + str.substring(end);
                    else
                        str = str.substring(0, start) + " " + insertCmd;
                }
                else
                {
                    str += nname + insertCmd;
                }
            }
        }
        else
        {
            str += nname + " " + insertCmd;
        }
        return PostCmd(str);
    }

    public static String GetProp(char PLUS, String str, String name, String...def)
    {
        str = PreCmd(PLUS, str);
        String nname = " " + PLUS + "set " + name + " ";
        String defVal = null != def && def.length > 0 ? def[0] : null;
        String val = defVal;
        int index = str.indexOf(nname);
        if (index != -1)
        {
            int start = index + nname.length();
            int end = str.indexOf(" " + PLUS, start);
            if (end != -1)
                val = str.substring(start, end).trim();
            else
                val = str.substring(start).trim();
            if (val.isEmpty()) // ""
                return defVal;
        }
        return val;
    }

    public static String RemoveProp(char PLUS, String str, String name, boolean[]...b)
    {
        str = PreCmd(PLUS, str);
        String nname = " " + PLUS + "set " + name;
        boolean res = false;
        int index = str.indexOf(nname);
        if (index != -1)
        {
            int start = index + nname.length();
            if(start == str.length()) // at last
            {
                str = str.substring(0, index);
            }
            else
            {
                nname += " ";
                start = str.indexOf(nname);
                if(start != -1)
                {
                    int end = str.indexOf(" " + PLUS, start + nname.length() - 1);
                    if (end != -1)
                        str = str.substring(0, start) + str.substring(end);
                    else
                        str = str.substring(0, start);
                    res = true;
                }
            }
        }
        if (null != b && b.length > 0 && null != b[0] && b[0].length > 0)
            b[0][0] = res;
        return PostCmd(str);
    }

    public static boolean IsProp(char PLUS, String str, String name)
    {
        str = PreCmd(PLUS, str);
        String nname = " " + PLUS + "set " + name;
        int start = str.indexOf(nname) + nname.length();
        if(start == str.length()) // at last
            return true;
        nname += " ";
        return (str.contains(nname));
    }

    public static Boolean GetBoolProp(char PLUS, final String str, String name, Boolean...def)
    {
        Boolean defVal = null != def && def.length > 0 ? def[0] : null;
        String defStr = null != defVal ? (defVal ? "1" : "0") : null;
        String val = GetProp(PLUS, str, name, defStr);
        return null != val ? strtob(val) : null;
    }

    public static String SetBoolProp(char PLUS, String str, String name, boolean val)
    {
        return SetProp(PLUS, str, name, btostr(val));
    }

    public static String RemoveParam(char PLUS, String str, String name, boolean[]...b)
    {
        str = PreCmd(PLUS, str);
        String nname = " " + PLUS + name;
        boolean res = false;
        int index = str.indexOf(nname);
        if (index != -1)
        {
            int start = index + nname.length();
            if(start == str.length()) // at last
            {
                str = str.substring(0, index);
                res = true;
            }
            else
            {
                nname += " ";
                start = str.indexOf(nname);
                if(start != -1)
                {
                    int end = str.indexOf(" " + PLUS, start + nname.length() - 1);
                    if (end != -1)
                        str = str.substring(0, start) + str.substring(end);
                    else
                        str = str.substring(0, start);
                    res = true;
                }
            }
        }
        if (b.length > 0 && null != b[0] && b[0].length > 0)
            b[0][0] = res;
        return PostCmd(str);
    }

    public static String SetParam(char PLUS, String str, String name, Object val)
    {
        str = PreCmd(PLUS, str);
        String nname = " " + PLUS + name;
        String insertCmd = val.toString().trim();
        int index = str.indexOf(nname);
        if (index != -1)
        {
            int start = index + nname.length();
            if(start == str.length()) // at last
            {
                str = str + " " + insertCmd;
            }
            else
            {
                nname += " ";
                start = str.indexOf(nname);
                if(start != -1)
                {
                    start += nname.length() - 1;
                    int end = str.indexOf(" " + PLUS, start);
                    if (end != -1)
                        str = str.substring(0, start) + " " + insertCmd + str.substring(end);
                    else
                        str = str.substring(0, start) + " " + insertCmd;
                }
                else
                {
                    str += nname + insertCmd;
                }
            }
        }
        else
        {
            str += nname + " " + insertCmd;
        }
        return PostCmd(str);
    }

    public static String SetCommand(char PLUS, String str, String name, boolean...prepend)
    {
        str = PreCmd(PLUS, str);
        String nname = " " + PLUS + name;
        boolean pp = null != prepend && prepend.length > 0 && prepend[0];
        int index = str.indexOf(nname);
        if (index == -1)
        {
            if(pp)
                str = nname + str;
            else
                str += nname;
        }
        else
        {
            index = index + nname.length();
            if (index < str.length() - 1 && str.charAt(index) != ' ')
            {
                if(pp)
                    str = nname + str;
                else
                    str += nname;
            }
        }
        return PostCmd(str);
    }

    public static String RemoveCommand(char PLUS, String str, String name, boolean[]...b)
    {
        str = PreCmd(PLUS, str);
        String nname = " " + PLUS + name;
        boolean res = false;
        int index = str.indexOf(nname);
        if (index != -1)
        {
            int start = index + nname.length();
            if(start == str.length()) // at last
            {
                str = str.substring(0, index);
                res = true;
            }
            else
            {
                if(str.charAt(start) == ' ')
                {
                    str = str.substring(0, index) + str.substring(start);
                    res = true;
                }
            }
        }
        if (b.length > 0 && null != b[0] && b[0].length > 0)
            b[0][0] = res;
        return PostCmd(str);
    }

    public static String GetParam(char PLUS, String str, String name, String...def)
    {
        str = PreCmd(PLUS, str);
        String nname = " " + PLUS + name + " ";
        String defVal = null != def && def.length > 0 ? def[0] : null;
        String val = defVal;
        int index = str.indexOf(nname);
        if (index != -1)
        {
            int start = index + nname.length() - 1;
            int end = str.indexOf(" " + PLUS, start);
            if (end != -1)
                val = str.substring(start, end).trim();
            else
                val = str.substring(start).trim();
            if (val.isEmpty()) // ""
                return defVal;
        }
        return val;
    }

    public static boolean HasParam(char PLUS, String str, String name)
    {
        str = PreCmd(PLUS, str);
        String nname = " " + PLUS + name;
        int start = str.indexOf(nname) + nname.length();
        if(start == str.length()) // at last
            return true;
        nname += " ";
        return (str.contains(nname));
    }

    public KidTechCommand(char PLUS, String str)
    {
        m_argPrefix = PLUS;
        m_cmd = str;
    }

    @Override
    public String toString()
    {
        return m_cmd;
    }

    public KidTechCommand SetProp(String name, Object val)
    {
        m_cmd = KidTechCommand.SetProp(m_argPrefix, m_cmd, name, val);
        return this;
    }

    public String Prop(String name, String...def)
    {
        return KidTechCommand.GetProp(m_argPrefix, m_cmd, name, def);
    }

    public KidTechCommand RemoveProp(String name, boolean[]...b)
    {
        m_cmd = KidTechCommand.RemoveProp(m_argPrefix, m_cmd, name, b);
        return this;
    }

    public boolean IsProp(String name)
    {
        return KidTechCommand.IsProp(m_argPrefix, m_cmd, name);
    }

    public Boolean GetBoolProp(String name, Boolean...def)
    {
        return KidTechCommand.GetBoolProp(m_argPrefix, m_cmd, name, def);
    }

    public KidTechCommand SetBoolProp(String name, boolean val)
    {
        return SetProp(name, btostr(val));
    }

    public KidTechCommand RemoveParam(String name, boolean[]...b)
    {
        m_cmd = KidTechCommand.RemoveParam(m_argPrefix, m_cmd, name, b);
        return this;
    }

    public KidTechCommand SetParam(String name, Object val)
    {
        m_cmd = KidTechCommand.SetParam(m_argPrefix, m_cmd, name, val);
        return this;
    }

    public KidTechCommand SetCommand(String name, boolean...prepend)
    {
        m_cmd = KidTechCommand.SetCommand(m_argPrefix, m_cmd, name, prepend);
        return this;
    }

    public KidTechCommand RemoveCommand(String name, boolean[]...b)
    {
        m_cmd = KidTechCommand.RemoveCommand(m_argPrefix, m_cmd, name, b);
        return this;
    }

    public String Param(String name, String...def)
    {
        return GetParam(m_argPrefix, m_cmd, name, def);
    }
}
