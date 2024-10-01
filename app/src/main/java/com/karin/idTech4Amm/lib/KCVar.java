package com.karin.idTech4Amm.lib;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class KCVar
{
    public static final String TYPE_NONE = "";
    public static final String TYPE_STRING = "string";
    public static final String TYPE_BOOL = "bool";
    public static final String TYPE_INTEGER = "integer";
    public static final String TYPE_FLOAT = "float";
    public static final String TYPE_VECTOR3 = "vector3";

    public static final int CATEGORY_CVAR = 1;
    public static final int CATEGORY_COMMAND = 2;

    public static final int FLAG_POSITIVE = 1;
    public static final int FLAG_NO_ARCHIVE = 1 << 1;

    public final String name;
    public final String type;
    public final String defaultValue;
    public final String description;
    public final Value[] values;
    public final int flags;
    public final int category;

    private KCVar(String name, String type, String defaultValue, String description, int category, int flags, Value[] values)
    {
        this.name = name;
        this.type = type;
        this.defaultValue = defaultValue;
        this.description = description;
        this.values = values;
        this.category = category;
        this.flags = flags;
    }

    public boolean HasFlag(int flag)
    {
        return (flags & flag) == flag;
    }

    public static class Value {
        public final String value;
        public final String desc;

        public Value(String value, String desc)
        {
            this.value = value;
            this.desc = desc;
        }
    }

    public static class Group {
        public final String name;
        public final boolean engine;
        public final List<KCVar> list;

        public Group(String name, boolean engine)
        {
            this.name = name;
            this.engine = engine;
            list = new ArrayList<>();
        }

        public Group AddCVar(KCVar...cvars)
        {
            if(null != cvars)
            {
                list.addAll(Arrays.asList(cvars));
            }
            return this;
        }
    }

    public static KCVar CreateCVar(String name, String type, String def, String desc, int flag, String...args)
    {
        List<Value> values = null;
        if(null != args && args.length >= 2)
        {
            values = new ArrayList<>();
            for(int i = 0; i < args.length - 1; i += 2)
                values.add(new Value(args[i], args[i + 1]));
        }
        return new KCVar(name, type, def, desc, CATEGORY_CVAR, flag, null != values ? values.toArray(new Value[0]) : null);
    }

    public static KCVar CreateCommand(String name, String type, String desc, int flag, String...args)
    {
        List<Value> values = null;
        if(null != args && args.length >= 2)
        {
            values = new ArrayList<>();
            for(int i = 0; i < args.length - 1; i += 2)
                values.add(new Value(args[i], args[i + 1]));
        }
        return new KCVar(name, type, "", desc, CATEGORY_COMMAND, flag, null != values ? values.toArray(new Value[0]) : null);
    }
}
