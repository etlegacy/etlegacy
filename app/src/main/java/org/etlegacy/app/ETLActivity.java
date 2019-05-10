package org.etlegacy.app;

import org.libsdl.app.SDLActivity;

public class ETLActivity extends SDLActivity
{
    @Override
    protected String[] getLibraries() {
        return new String[] {
                "etl"
        };
    }
}
