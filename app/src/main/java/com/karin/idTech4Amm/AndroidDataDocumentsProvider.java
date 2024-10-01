package com.karin.idTech4Amm;

import com.karin.idTech4Amm.misc.HarmDocumentsProvider;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;
import com.etlegacy.app.R;

// Document provider: /sdcard/Android/data/<package_name>/files
public class AndroidDataDocumentsProvider extends HarmDocumentsProvider
{
    @Override
    protected String GetPath()
    {
        return Q3EUtils.GetAppStoragePath(getContext(), null);
    }

    @Override
    protected String GetName()
    {
        return Q3ELang.tr(getContext(), R.string.external_storage);
    }
}
