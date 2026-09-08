set(IRBIS_APP irbis)

add_executable(${IRBIS_APP}
        app/main.cpp
        app/mainwindow.cpp
        ui/mainwindow.ui
        data/icons/irbis.qrc
)

if (WIN32)
    set(IRBIS_WIN_ICON "${CMAKE_CURRENT_SOURCE_DIR}/data/icons/irbis.rc")
    target_sources(${IRBIS_APP} PRIVATE "${IRBIS_WIN_ICON}")
    set_source_files_properties("${IRBIS_WIN_ICON}" PROPERTIES
            OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/icons/irbis.ico"
            COMPILE_FLAGS "--include-dir=${CMAKE_CURRENT_SOURCE_DIR}/data/icons")
endif ()

target_include_directories(${IRBIS_APP} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/app
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${IRBIS_APP} PRIVATE irbis::irbis)

irbis_apply_flags(${IRBIS_APP})

if (CCACHE_FOUND)
    set_property(TARGET ${IRBIS_APP} PROPERTY CXX_COMPILER_LAUNCHER ccache)
endif ()

set(IRBIS_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
if (EXISTS "${IRBIS_DATA_DIR}")
    add_custom_command(TARGET ${IRBIS_APP} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${IRBIS_APP}>/data"
                "$<TARGET_FILE_DIR:${IRBIS_APP}>/styles"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${IRBIS_DATA_DIR}"
                "$<TARGET_FILE_DIR:${IRBIS_APP}>/data"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${IRBIS_DATA_DIR}/styles"
                "$<TARGET_FILE_DIR:${IRBIS_APP}>/styles"
            COMMENT "Copying data/ (styles, fonts) next to irbis"
            VERBATIM
    )
endif ()

option(IRBIS_BUILD_TESTS "Build unit tests (nice-axis, tf-builder, …)" ON)
if (IRBIS_BUILD_TESTS)
    enable_testing()

    add_executable(nice_axis_test tests/nice-axis-test.cpp)
    target_include_directories(nice_axis_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_features(nice_axis_test PRIVATE cxx_std_23)
    add_test(NAME nice_axis_test COMMAND nice_axis_test)

    add_executable(tf_builder_test tests/tf-builder-test.cpp)
    target_include_directories(tf_builder_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_features(tf_builder_test PRIVATE cxx_std_23)
    target_link_libraries(tf_builder_test PRIVATE numina::numina)
    add_test(NAME tf_builder_test COMMAND tf_builder_test)

    add_executable(tf_stepper_test tests/tf-stepper-test.cpp)
    target_include_directories(tf_stepper_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_features(tf_stepper_test PRIVATE cxx_std_23)
    target_link_libraries(tf_stepper_test PRIVATE numina::numina)
    add_test(NAME tf_stepper_test COMMAND tf_stepper_test)

    add_executable(data_file_parser_test tests/data-file-parser-test.cpp)
    target_include_directories(data_file_parser_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_features(data_file_parser_test PRIVATE cxx_std_23)
    target_link_libraries(data_file_parser_test PRIVATE Qt6::Core)
    add_test(NAME data_file_parser_test COMMAND data_file_parser_test)
endif ()
