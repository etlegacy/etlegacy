package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;

import com.etlegacy.app.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;

public final class SupportDeveloperFunc extends GameLauncherFunc
{
    public SupportDeveloperFunc(GameLauncher gameLauncher)
    {
        super(gameLauncher);
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        run();
    }

    public void run()
    {
        Support_fdroid();
    }

    private void Support_fdroid()
    {
        AlertDialog.Builder bldr=new AlertDialog.Builder(m_gameLauncher);
        bldr.setTitle(R.string.do_you_want_to_support_the_developer);
        bldr.setPositiveButton(Q3ELang.tr(m_gameLauncher, R.string.donate_to) + "F-Droid", new AlertDialog.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                ContextUtility.OpenUrlExternally(m_gameLauncher, "https://f-droid.org/donate/");
                dialog.dismiss();
            }
        });
        bldr.setNeutralButton(Q3ELang.tr(m_gameLauncher, R.string.more_apps_in) + "F-Droid", new AlertDialog.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                if(!ContextUtility.OpenApp(m_gameLauncher, "org.fdroid.fdroid"))
                {
                    ContextUtility.OpenUrlExternally(m_gameLauncher, "https://f-droid.org/packages/");
                    dialog.dismiss();
                }
            }
        });
        bldr.setNegativeButton(R.string.dont_ask, new AlertDialog.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        });
        AlertDialog dl=bldr.create();
        dl.setCancelable(false);
        dl.show();
    }

    private void Support_n0n3m4()
    {
        /*
        AlertDialog.Builder bldr=new AlertDialog.Builder(m_gameLauncher);
        bldr.setTitle(R.string.do_you_want_to_support_the_developer);
        bldr.setPositiveButton("Donate by PayPal", new AlertDialog.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                Intent ppIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=kosleb1169%40gmail%2ecom&lc=US&item_name=n0n3m4&no_note=0&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donate_SM%2egif%3aNonHostedGuest"));
                ppIntent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                m_gameLauncher.startActivity(ppIntent);
                dialog.dismiss();
            }
        });
        bldr.setNeutralButton("More apps by n0n3m4", new AlertDialog.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                Intent marketIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("market://search?q=pub:n0n3m4"));
                marketIntent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                m_gameLauncher.startActivity(marketIntent);
                dialog.dismiss();
            }
        });
        AlertDialog dl=bldr.create();
        dl.setCancelable(false);
        dl.show();
        */
    }
}
