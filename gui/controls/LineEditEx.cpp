#include "stdafx.h"
#include "LineEditEx.h"
#include "ContextMenu.h"
#include "../utils/utils.h"
#include "../styles/ThemeParameters.h"
#include "../main_window/history_control/MessageStyle.h"


namespace Ui
{
    LineEditEx::LineEditEx(QWidget* _parent, const Options &_options)
        : QLineEdit(_parent)
        , options_(_options)
    {
        installEventFilter(this);
        changeTextColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        connect(this, &QLineEdit::textChanged, this, [=]()
        {
            changeTextColor(Styling::getParameters().getColor(text().length() > 0 ? Styling::StyleVariable::TEXT_SOLID : Styling::StyleVariable::BASE_PRIMARY));
        });
    }

    void LineEditEx::setCustomPlaceholder(const QString& _text)
    {
        customPlaceholder_ = _text;
    }

    void LineEditEx::setCustomPlaceholderColor(const QColor& _color)
    {
        customPlaceholderColor_ = _color;
    }

    void LineEditEx::changeTextColor(const QColor & _color)
    {
        QPalette pal(palette());
        pal.setColor(QPalette::Text, _color);
        pal.setColor(QPalette::Highlight, MessageStyle::getTextSelectionColor());
        pal.setColor(QPalette::HighlightedText, MessageStyle::getSelectionFontColor());
        setPalette(pal);
    }

    QMenu *LineEditEx::contextMenu()
    {
        return contextMenu_;
    }

    void LineEditEx::setOptions(LineEditEx::Options _options)
    {
        options_ = _options;
    }

    void LineEditEx::focusInEvent(QFocusEvent* _event)
    {
        QLineEdit::focusInEvent(_event);
        Q_EMIT focusIn();
    }

    void LineEditEx::focusOutEvent(QFocusEvent* _event)
    {
        QLineEdit::focusOutEvent(_event);

        if (_event->reason() != Qt::ActiveWindowFocusReason)
            Q_EMIT focusOut();
    }

    void LineEditEx::mousePressEvent(QMouseEvent* _event)
    {
        Q_EMIT clicked(_event->button());
        QLineEdit::mousePressEvent(_event);
    }

    void LineEditEx::keyPressEvent(QKeyEvent* _event)
    {
        if (_event->key() == Qt::Key_Backspace && text().isEmpty())
            Q_EMIT emptyTextBackspace();

        if (_event->key() == Qt::Key_Escape && !_event->isAutoRepeat())
            Q_EMIT escapePressed();

        if (_event->key() == Qt::Key_Up)
            Q_EMIT upArrow();

        if (_event->key() == Qt::Key_Down)
            Q_EMIT downArrow();

        if ((_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return) &&
            (_event->modifiers() == Qt::NoModifier || _event->modifiers() == Qt::KeypadModifier))
            Q_EMIT enter();

        if (options_.noPropagateKeys_.find(_event->key()) != options_.noPropagateKeys_.end())
            return;

        if (platform::is_apple() && _event->key() == Qt::Key_Backspace && (_event->modifiers() & Qt::ControlModifier))
        {
            auto t = text();
            auto end = (selectedText() == t) ? t.length() : cursorPosition();
            t.remove(0, end);
            setText(t);
            setCursorPosition(0);
            return;
        }

        if (_event->matches(QKeySequence::DeleteStartOfWord) && hasSelectedText())
            _event->setModifiers(Qt::NoModifier);

        QLineEdit::keyPressEvent(_event);
    }

    void LineEditEx::contextMenuEvent(QContextMenuEvent *e)
    {
        contextMenu_ = createStandardContextMenu();
        ContextMenu::applyStyle(contextMenu_, false, Utils::scale_value(15), Utils::scale_value(36));
        contextMenu_->exec(e->globalPos());
        contextMenu_->deleteLater();
        contextMenu_ = nullptr;
    }

    void LineEditEx::paintEvent(QPaintEvent *_event)
    {
        if (!customPlaceholder_.isEmpty() && text().isEmpty())
        {
            // copied from QLineEdit::paintEvent
            QPainter p(this);
            QStyleOptionFrame panel;
            initStyleOption(&panel);
            QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
            const auto m = RenderMargins::ContentMargin == margins_ ? contentsMargins() : textMargins();
            r.setX(r.x() + m.left());
            r.setY(r.y() + m.top());
            r.setRight(r.right() - m.right());
            r.setBottom(r.bottom() - m.bottom());

            QFontMetrics fm = fontMetrics();
            const auto vscroll = r.y() + (r.height() - fm.height() + 1) / 2;

            const int horMargin = 2;
            QRect lineRect(r.x() + horMargin, vscroll, r.width() - 2*horMargin, fm.height());

            p.setPen(customPlaceholderColor_);
            p.setFont(font());
            QString elidedText = fm.elidedText(customPlaceholder_, Qt::ElideRight, lineRect.width());
            p.drawText(lineRect, elidedText);
        }
        QLineEdit::paintEvent(_event);
    }

    bool LineEditEx::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::ShortcutOverride)
        {
            auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
            {
                keyEvent->accept();
                return true;
            }
        }
        else if (_event->type() == QEvent::KeyPress)
        {
            auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
            {
                if (keyEvent->key() == Qt::Key_Tab)
                    Q_EMIT tab();
                else
                    Q_EMIT backtab();

                if (options_.noPropagateKeys_.find(keyEvent->key()) != options_.noPropagateKeys_.end())
                {
                    keyEvent->accept();
                    return true;
                }
            }
        }

        return false;
    }

    void LineEditEx::wheelEvent(QWheelEvent* _event)
    {
        int step = 0;

        if constexpr (platform::is_apple())
        {
            auto numPixels = _event->pixelDelta();
            auto numDegrees = _event->angleDelta() / 8;
            if (!numPixels.isNull())
                step = numPixels.y();
            else if (!numDegrees.isNull())
                step = numDegrees.y() / 15;
        }
        else
        {
            const auto numDegrees = _event->delta() / 8;
            step = numDegrees / 15;
        }

        step /= Utils::scale_bitmap(1);
        Q_EMIT scrolled(step, QPrivateSignal());
        QWidget::wheelEvent(_event);
    }

    void LineEditEx::setRenderMargins(RenderMargins _margins)
    {
        if (margins_ != _margins)
        {
            margins_ = _margins;
            update();
        }
    }
}
