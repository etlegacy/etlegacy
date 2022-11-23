#-----------------------------------------------------------------
# Legacy banner for extra headshots
#-----------------------------------------------------------------

function(colored_msg msg)
    if(NOT WIN32)
        string(ASCII 27 Esc)
        message("${Esc}[31m${msg}${Esc}[m")
    else()
        message("${msg}")
    endif()
endfunction()

macro(print_legacy_banner)
    colored_msg("")
    colored_msg("  ███████╗████████╗   ██╗     ███████╗ ██████╗  █████╗  ██████╗██╗   ██╗")
    colored_msg("  ██╔════╝╚══██╔══╝██╗██║     ██╔════╝██╔════╝ ██╔══██╗██╔════╝╚██╗ ██╔╝")
    colored_msg("  █████╗     ██║   ╚═╝██║     █████╗  ██║  ███╗███████║██║      ╚████╔╝")
    colored_msg("  ██╔══╝     ██║   ██╗██║     ██╔══╝  ██║   ██║██╔══██║██║       ╚██╔╝")
    colored_msg("  ███████╗   ██║   ╚═╝███████╗███████╗╚██████╔╝██║  ██║╚██████╗   ██║")
    colored_msg("  ╚══════╝   ╚═╝      ╚══════╝╚══════╝ ╚═════╝ ╚═╝  ╚═╝ ╚═════╝   ╚═╝")
    colored_msg("")
endmacro()

macro(print_header)
    print_legacy_banner()
endmacro()
