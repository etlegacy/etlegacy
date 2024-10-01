package com.karin.idTech4Amm;

import com.etlegacy.app.R;
import android.os.Build;

import com.karin.idTech4Amm.misc.HarmDocumentsProvider;
import com.n0n3m4.q3e.Q3ELang;

// Document provider: /data/user/<user>
public class DataUserDocumentsProvider extends HarmDocumentsProvider
{
    @Override
    protected String GetPath()
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
        {
            return getContext().getDataDir().getAbsolutePath();
        }
        else
        {
            return getContext().getCacheDir().getAbsolutePath();
        }
    }

    @Override
    protected String GetName()
    {
        return Q3ELang.tr(getContext(), R.string.internal_storage);
    }
}
