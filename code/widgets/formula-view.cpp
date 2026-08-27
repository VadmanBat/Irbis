#include "code/widgets/formula-view.h"

FormulaView::FormulaView(QWidget* parent) : QLabel(parent) {
    setObjectName(QStringLiteral("FormulaView"));
    setTextFormat(Qt::RichText);
    setWordWrap(true);
    setMargin(0);
    setIndent(0);
    setContentsMargins(2, 4, 2, 4);
    setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    setTextInteractionFlags(Qt::TextSelectableByMouse);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setOpenExternalLinks(false);
}

void FormulaView::setHtml(const QString& html) {
    html_ = html.isEmpty() ? QStringLiteral("0") : html;
    setText(html_);
}
