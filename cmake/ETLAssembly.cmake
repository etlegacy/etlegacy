#-----------------------------------------------------------------
# Assembly
#-----------------------------------------------------------------

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND ETL_64BITS AND ETL_ENABLE_SSE ) #ENABLE_SSE
	message(STATUS "Will use external asm files for specific functions")
    enable_language(ASM_MASM)

    LIST(APPEND COMMON_SRC "src/qcommon/asm/*.asm")
    # not sure if setting this specific properties is necesarry here
    foreach(ASM ${COMMON_SRC})
        if(set(ASM asm))
            get_filename_component(OUTPUT_FILE_WE ${ASM} NAME_ASM)
            set_source_files_properties(${NAME_ASM} PROPERTIESCOMPILE_FLAGS "/safeseh")
        endif()
	endforeach()

else()
	message(STATUS "No further steps required as its required for 64bits Windows with Enabled SSE at this moment")
endif()