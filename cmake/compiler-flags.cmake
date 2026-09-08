function(irbis_apply_flags target)
    if (NOT TARGET ${target})
        return()
    endif ()

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${target} PRIVATE
                -O0
                -g
                -fno-inline
                -Wall -Wextra -Wpedantic
                -Wshadow
                -Wconversion
        )
        target_compile_definitions(${target} PRIVATE DEBUG)
    elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${target} PRIVATE
                -O3
                -funroll-loops
                -fvisibility=hidden
                -fipa-pta
                -fdevirtualize
        )
        target_compile_definitions(${target} PRIVATE
                NDEBUG
                QT_NO_DEBUG_OUTPUT
                QT_NO_DEBUG
                QT_NO_WARNING_OUTPUT
                QT_USE_QSTRINGBUILDER
        )
    endif ()

    set_target_properties(${target} PROPERTIES
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON
    )

    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(${target} PROPERTIES QT_CONFIG release)
    endif ()

    get_target_property(_irbis_type ${target} TYPE)
    if (STATIC_BUILD AND _irbis_type STREQUAL "EXECUTABLE")
        set_target_properties(${target} PROPERTIES
                LINK_SEARCH_START_STATIC ON
                LINK_SEARCH_END_STATIC ON
                CXX_VISIBILITY_PRESET hidden
        )
        target_link_options(${target} PRIVATE -static)
        target_compile_definitions(${target} PRIVATE QT_STATICPLUGIN)
    endif ()
endfunction()
