#-----------------------------------------------------------------
# Assembly
#-----------------------------------------------------------------

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND ETL_64BITS AND ETL_ENABLE_SSE ) #ENABLE_SSE
	message(STATUS "Will use external asm files for specific functions")
    enable_language(ASM_MASM)

    # Gather list of all .asm files"
    file(GLOB ASM_FILES ${CMAKE_SOURCE_DIR}/src/qcommon/asm/*.asm)

    foreach(ASM_FILE ${ASM_FILES})
        list(APPEND COMMON_SRC ${ASM_FILE})
        # not sure if setting this specific properties is necesarry here
        set_source_files_properties(${ASM_FILE} PROPERTIESCOMPILE_FLAGS "/safeseh")
    endforeach()

else()
	message(STATUS "No further steps required as its required for 64bits Windows with Enabled SSE at this moment")
endif()