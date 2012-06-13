function(force_rescan_library LIBRARY_NAME)
        set(${LIBRARY_NAME}_LIBRARY "${LIBRARY_NAME}_LIBRARY-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
endfunction()
