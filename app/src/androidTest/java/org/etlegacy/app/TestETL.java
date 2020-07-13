package org.etlegacy.app;

import android.app.Application;
import android.app.Instrumentation;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.util.DisplayMetrics;

import com.robotium.solo.Solo;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

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
    Instrumentation instrumentation;
    InstrumentationRegistry instrumentationRegistry;
    Application app;

    @Before
    public void setUp() throws Exception {
        app = (Application) instrumentationRegistry.getTargetContext().getApplicationContext();

        instrumentation = new Instrumentation();
        instrumentation = instrumentationRegistry.getInstrumentation();
        instrumentation.runOnMainSync(new Runnable() {
            @Override
            public void run() {
                instrumentation.callApplicationOnCreate(app);
            }
        });
    }

    @Test
    public void clickOnNickEntry() throws Exception {
        // Perform click on Nickname Entry
        // TODO: Implement

        assertEquals("org.etlegacy.app", app.getPackageName());
        DisplayMetrics displaymetrics = new DisplayMetrics();

        if (app.getApplicationContext().getClass().getSimpleName() == "ETLActivity") {
            sleep(10000);
            solo.drag(0, displaymetrics.widthPixels, 0, displaymetrics.heightPixels / 2, 100);
        }

    }

}