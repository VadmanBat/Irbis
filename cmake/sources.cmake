set(irbis_SOURCES
        src/irbis/style.cpp

        src/irbis/tabs/id-tab.cpp
        src/irbis/tabs/id-tab-run.cpp
        src/irbis/tabs/id-tab-identify.cpp
        src/irbis/tabs/analysis-tab.cpp
        src/irbis/tabs/synthesis-tab.cpp
        src/irbis/tabs/synthesis-tab-run.cpp
        src/irbis/tabs/synthesis-tab-synth.cpp
        src/irbis/tabs/synthesis-tab-face.cpp
        src/irbis/tabs/rim-tab.cpp
        src/irbis/tabs/rim-tab-run.cpp

        src/irbis/control/controller-design.cpp

        src/irbis/dialogs/mod-par-dialog.cpp
        src/irbis/dialogs/help-dialog.cpp
        src/irbis/dialogs/chart-dialog.cpp
        src/irbis/dialogs/tran-func-dialog.cpp
        src/irbis/dialogs/tran-func-dialog-poles.cpp
        src/irbis/dialogs/slider-settings-dialog.cpp
        src/irbis/dialogs/chart-viewer/chart-viewer-window.cpp
        src/irbis/dialogs/chart-viewer/chart-viewer-window-ui.cpp

        src/irbis/widgets/double-slider.cpp
        src/irbis/widgets/reg-parameter.cpp
        src/irbis/widgets/reg-parameter-range.cpp
        src/irbis/widgets/regulation-widget.cpp
        src/irbis/widgets/tf-display-widget.cpp
        src/irbis/widgets/formula-view.cpp
        src/irbis/widgets/tf-form/tran-func-form.cpp
        src/irbis/widgets/tf-form/tran-func-form-edit.cpp
        src/irbis/widgets/tf-form/tran-func-form-io.cpp
        src/irbis/widgets/tf-form/tran-func-form-name.cpp

        src/irbis/charts/chart-panel.cpp
        src/irbis/charts/c0-c1-chart.cpp
        src/irbis/charts/c0-c1-chart-axes.cpp
        src/irbis/charts/c0-c1-chart-pointer.cpp
        src/irbis/charts/response-chart-bank.cpp
        src/irbis/charts/response-chart-bank-data.cpp
        src/irbis/charts/response-chart-bank-channels.cpp
        src/irbis/charts/response-chart-bank-view.cpp
        src/irbis/charts/interactive-chart-view.cpp
        src/irbis/charts/utils/chart-utils.cpp
        src/irbis/charts/utils/chart-utils-axes.cpp
        src/irbis/charts/utils/chart-utils-axes-grid.cpp
        src/irbis/charts/utils/chart-utils-series.cpp
        src/irbis/charts/utils/chart-utils-menu.cpp
        src/irbis/charts/utils/chart-utils-export.cpp

        ui/tabs/id-tab.ui
        ui/tabs/analysis-tab.ui
        ui/tabs/synthesis-tab.ui
        ui/tabs/rim-tab.ui
        ui/dialogs/mod-par-dialog.ui
        ui/dialogs/help-dialog.ui
        ui/dialogs/chart-dialog.ui
        ui/dialogs/tran-func-dialog.ui
        ui/dialogs/slider-settings-dialog.ui

        data/irbis_resources.qrc
)
