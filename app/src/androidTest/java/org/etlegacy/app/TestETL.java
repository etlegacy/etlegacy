package org.etlegacy.app;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;
import com.robotium.solo.Solo;

import static android.os.SystemClock.sleep;
import static org.junit.Assert.assertEquals;

/**
 * Instrumentation test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class TestETL {

    private Solo solo;

    public void useAppContext() throws Exception {
        // Context of the app under test.
        Context appContext = InstrumentationRegistry.getTargetContext();

        assertEquals("org.etlegacy.app", appContext.getPackageName());
    }

    @Test
    public void clickOnNickEntry() throws Exception {
        // Perform click on Nickname Entry
        Context context = ETLActivity.this;

        if (!((Activity) context).isFinishing()) {
            sleep(10000);
            solo.drag(0, Resources.getSystem().displayMetrics.widthPixels, 0, Resources.getSystem().displayMetrics.heighPixels / 2, 100);
//            solo.clickOnScreen(Resources.getSystem().displayMetrics.widthPixels, Resources.getSystem().displayMetrics.heighPixels, 1);
        }
    }

}