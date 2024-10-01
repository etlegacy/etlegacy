package com.n0n3m4.q3e.karin;

public abstract class KResultRunnable implements Runnable
{
    private boolean m_result = false;

    protected abstract boolean Run();

    @Override
    public void run()
    {
        SetResult(Run());
    }

    public boolean GetResult()
    {
        boolean res = m_result;
        m_result = false;
        return res;
    }

    protected void SetResult(boolean result)
    {
        m_result = result;
    }
}
