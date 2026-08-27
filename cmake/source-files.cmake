set(UI_FILES
        ${CMAKE_SOURCE_DIR}/ui/mainwindow.ui
        ${CMAKE_SOURCE_DIR}/ui/tabs/id-tab.ui
        ${CMAKE_SOURCE_DIR}/ui/tabs/analysis-tab.ui
        ${CMAKE_SOURCE_DIR}/ui/tabs/synthesis-tab.ui
        ${CMAKE_SOURCE_DIR}/ui/tabs/rim-tab.ui
        ${CMAKE_SOURCE_DIR}/ui/dialogs/mod-par-dialog.ui
        ${CMAKE_SOURCE_DIR}/ui/dialogs/help-dialog.ui
        ${CMAKE_SOURCE_DIR}/ui/dialogs/chart-dialog.ui
        ${CMAKE_SOURCE_DIR}/ui/dialogs/tran-func-dialog.ui
        ${CMAKE_SOURCE_DIR}/ui/dialogs/slider-settings-dialog.ui
)

set(SOURCES
        ${CMAKE_SOURCE_DIR}/main.cpp
        ${CMAKE_SOURCE_DIR}/data/icons/irbis.qrc

        ${CMAKE_SOURCE_DIR}/code/app/mainwindow.cpp

        ${CMAKE_SOURCE_DIR}/code/tabs/id-tab.cpp
        ${CMAKE_SOURCE_DIR}/code/tabs/id-tab-run.cpp
        ${CMAKE_SOURCE_DIR}/code/tabs/id-tab-identify.cpp
        ${CMAKE_SOURCE_DIR}/code/tabs/analysis-tab.cpp
        ${CMAKE_SOURCE_DIR}/code/tabs/synthesis-tab.cpp
        ${CMAKE_SOURCE_DIR}/code/tabs/synthesis-tab-run.cpp
        ${CMAKE_SOURCE_DIR}/code/tabs/synthesis-tab-synth.cpp
        ${CMAKE_SOURCE_DIR}/code/tabs/synthesis-tab-face.cpp
        ${CMAKE_SOURCE_DIR}/code/tabs/rim-tab.cpp
        ${CMAKE_SOURCE_DIR}/code/tabs/rim-tab-run.cpp

        ${CMAKE_SOURCE_DIR}/code/control/controller-design.cpp

        ${CMAKE_SOURCE_DIR}/code/dialogs/mod-par-dialog.cpp
        ${CMAKE_SOURCE_DIR}/code/dialogs/help-dialog.cpp
        ${CMAKE_SOURCE_DIR}/code/dialogs/chart-dialog.cpp
        ${CMAKE_SOURCE_DIR}/code/dialogs/tran-func-dialog.cpp
        ${CMAKE_SOURCE_DIR}/code/dialogs/tran-func-dialog-poles.cpp
        ${CMAKE_SOURCE_DIR}/code/dialogs/slider-settings-dialog.cpp
        ${CMAKE_SOURCE_DIR}/code/dialogs/chart-viewer/chart-viewer-window.cpp
        ${CMAKE_SOURCE_DIR}/code/dialogs/chart-viewer/chart-viewer-window-ui.cpp

        ${CMAKE_SOURCE_DIR}/code/widgets/double-slider.cpp
        ${CMAKE_SOURCE_DIR}/code/widgets/reg-parameter.cpp
        ${CMAKE_SOURCE_DIR}/code/widgets/reg-parameter-range.cpp
        ${CMAKE_SOURCE_DIR}/code/widgets/regulation-widget.cpp
        ${CMAKE_SOURCE_DIR}/code/widgets/tf-display-widget.cpp
        ${CMAKE_SOURCE_DIR}/code/widgets/formula-view.cpp
        ${CMAKE_SOURCE_DIR}/code/widgets/tf-form/tran-func-form.cpp
        ${CMAKE_SOURCE_DIR}/code/widgets/tf-form/tran-func-form-edit.cpp
        ${CMAKE_SOURCE_DIR}/code/widgets/tf-form/tran-func-form-io.cpp
        ${CMAKE_SOURCE_DIR}/code/widgets/tf-form/tran-func-form-name.cpp

        ${CMAKE_SOURCE_DIR}/code/charts/chart-panel.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/c0-c1-chart.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/c0-c1-chart-axes.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/c0-c1-chart-pointer.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/response-chart-bank.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/response-chart-bank-data.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/response-chart-bank-channels.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/response-chart-bank-view.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/interactive-chart-view.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/utils/chart-utils.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/utils/chart-utils-axes.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/utils/chart-utils-axes-grid.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/utils/chart-utils-series.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/utils/chart-utils-menu.cpp
        ${CMAKE_SOURCE_DIR}/code/charts/utils/chart-utils-export.cpp

        ${UI_FILES}
)
