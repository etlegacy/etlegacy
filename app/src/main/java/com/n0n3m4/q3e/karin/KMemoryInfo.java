package com.n0n3m4.q3e.karin;

import android.app.ActivityManager;
import android.content.Context;
import android.os.Build;
import android.os.Debug;
import android.os.Process;

public final class KMemoryInfo
{
    private static final int CONST_KB = 1024;
    private static final int CONST_MB = 1024 * 1024;

    public long java_memory;
    public long native_memory;
    public long graphics_memory;
    public long total_memory;
    public long avail_memory;
    public long used_memory;
    public long stack_memory;

    public void Get(ActivityManager am, int[] processes, ActivityManager.MemoryInfo outInfo)
    {
        am.getMemoryInfo(outInfo);

        avail_memory = outInfo.availMem;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) // 16
        {
            total_memory = outInfo.totalMem;
            used_memory = outInfo.totalMem - outInfo.availMem;
        }

        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.P)
        {
            Debug.MemoryInfo memoryInfo = new Debug.MemoryInfo();
            Debug.getMemoryInfo(memoryInfo);
            java_memory = strtol(memoryInfo.getMemoryStat("summary.java-heap")) * CONST_KB;
            native_memory = strtol(memoryInfo.getMemoryStat("summary.native-heap")) * CONST_KB;
            //String code = memoryInfo.getMemoryStat("summary.code");
            stack_memory = strtol(memoryInfo.getMemoryStat("summary.stack")) * CONST_KB;
            graphics_memory = strtol(memoryInfo.getMemoryStat("summary.graphics")) * CONST_KB;
            //String privateOther = memoryInfo.getMemoryStat("summary.private-other");
            //String system = memoryInfo.getMemoryStat("summary.system");
            //String swap = memoryInfo.getMemoryStat("summary.total-swap");
        }
        else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) // 23 // > Android P, slow frequency
        {
            Debug.MemoryInfo[] memInfos = am.getProcessMemoryInfo(processes);
            Debug.MemoryInfo memInfo = memInfos[0];
            java_memory = strtol(memInfo.getMemoryStat("summary.java-heap")) * CONST_KB;
            native_memory = strtol(memInfo.getMemoryStat("summary.native-heap")) * CONST_KB;
            graphics_memory = strtol(memInfo.getMemoryStat("summary.graphics")) * CONST_KB;
            stack_memory = strtol(memInfo.getMemoryStat("summary.stack")) * CONST_KB;
            //String code_mem = memInfo.getMemoryStat("summary.code");
            //String others_mem = memInfo.getMemoryStat("summary.system");
        }
        else
        {
            Debug.MemoryInfo[] memInfos = am.getProcessMemoryInfo(processes);
            Debug.MemoryInfo memInfo = memInfos[0];
            java_memory = memInfo.dalvikPrivateDirty * CONST_KB;
            native_memory = memInfo.nativePrivateDirty * CONST_KB;
        }
    }

    public void Get(ActivityManager am)
    {
        final int[] processes = { Process.myPid() };
        ActivityManager.MemoryInfo outInfo = new ActivityManager.MemoryInfo();
        Get(am, processes, outInfo);
    }

    public void Get(Context context)
    {
        Get((ActivityManager)(context.getSystemService(Context.ACTIVITY_SERVICE)));
    }

    public void Reset()
    {
        java_memory = native_memory = graphics_memory = stack_memory
                = avail_memory = total_memory = used_memory = 0;
    }

    public void Invalid()
    {
        java_memory = native_memory = graphics_memory = stack_memory
                = avail_memory = total_memory = used_memory = -1;
    }

    public void Kb()
    {
        Unit(CONST_KB);
    }

    public void Mb()
    {
        Unit(CONST_MB);
    }

    private void Unit(int unit)
    {
        if(java_memory > 0)
            java_memory = java_memory / unit;
        if(native_memory > 0)
            native_memory = native_memory / unit;
        if(graphics_memory > 0)
            graphics_memory = graphics_memory / unit;
        if(stack_memory > 0)
            stack_memory = stack_memory / unit;
        if(avail_memory > 0)
            avail_memory = avail_memory / unit;
        if(total_memory > 0)
            total_memory = total_memory / unit;
        if(used_memory > 0)
            used_memory = used_memory / unit;
    }

    private long strtol(String str)
    {
        if (null == str || str.isEmpty())
            return 0L;
        try
        {
            return Long.parseLong(str);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return 0;
        }
    }
}
