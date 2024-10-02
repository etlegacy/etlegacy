#-----------------------------------------------------------------
# Q3eloader
#-----------------------------------------------------------------

FILE(GLOB Q3e_SRC
        "app/cpp/q3e.c"
        "app/cpp/eventqueue.cpp"
)

set(q3e_lib
        log
        android
        )

add_library( # Sets the name of the library.
        q3eloader

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${Q3e_SRC}
        )


target_link_libraries( # Specifies the target library.
        q3eloader

        # Links the target library to the log library
        # included in the NDK.
        #${log-lib}
        ${q3e_lib}
        )
