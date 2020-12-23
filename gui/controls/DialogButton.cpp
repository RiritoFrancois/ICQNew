#include "stdafx.h"
#include "../styles/ThemeParameters.h"
#include "../utils/utils.h"
#include "DialogButton.h"
#include "../fonts.h"

namespace
{
    QColor StyleColor(Styling::StyleVariable _var)
    {
        return Styling::getParameters().getColor(_var);
    }

    int defaultButtonWidth()
    {
        return Utils::scale_value(80);
    }

    int defaultButtonHeight()
    {
        return platform::is_apple() ? Utils::scale_value(33) : Utils::scale_value(32);
    }

    class ProxyStyle : public QProxyStyle
    {
    public:
        QSize sizeFromContents(ContentsType ct, const QStyleOption* opt, const QSize & csz, const QWidget* widget = 0) const override
        {
            QSize sz = QProxyStyle::sizeFromContents(ct, opt, csz, widget);
            if (ct == ContentsType::CT_PushButton)
                sz.rwidth() += Utils::scale_value(40);

            return sz;
        }
    };

    QFont buttonFont()
    {
        return Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold);
    }
}

namespace Ui
{
    DialogButton::DialogButton(QWidget* _parent, const QString _text, const DialogButtonRole _role)
        : QPushButton(_parent)
        , hovered_(false)
        , pressed_(false)
        , text_(_text)
        , initRole_(_role)
        , bgColor_(Qt::transparent)
        , bgColorHover_(Qt::transparent)
        , bgColorPress_(Qt::transparent)
        , borderColor_(Qt::transparent)
        , borderColorHover_(Qt::transparent)
        , borderColorPress_(Qt::transparent)
        , textColor_(Qt::transparent)
        , textColorHover_(Qt::transparent)
        , textColorPress_(Qt::transparent)
    {
        Utils::SetProxyStyle(this, new ProxyStyle());
        setFocusPolicy(Qt::NoFocus);
        setMouseTracking(true);
        setFlat(true);

        setFixedHeight(defaultButtonHeight());

        changeRole(initRole_);

        QFontMetrics metrics(buttonFont());
        setMinimumWidth(std::max(defaultButtonWidth(), metrics.horizontalAdvance(text_) + 2*Utils::scale_value(10)));
        setFont(buttonFont());
        setText(text_);
    }

    void DialogButton::changeRole(const DialogButtonRole _role)
    {
        role_ = _role;
        setCursor(!isEnabled() ? Qt::ArrowCursor : Qt::PointingHandCursor);
        setBackgroundColor();
        setBorderColor();
        setTextColor();
        updateTextColor();
        update();
    }

    bool DialogButton::isEnabled() const
    {
        return role_ != DialogButtonRole::DISABLED;
    }

    void DialogButton::setEnabled(bool _isEnabled)
    {
        changeRole(_isEnabled ? initRole_ : DialogButtonRole::DISABLED);
    }

    void DialogButton::updateWidth()
    {
        QFontMetrics metrics(font());
        const auto margins = contentsMargins();
        setFixedWidth(metrics.horizontalAdvance(text()) + margins.left() + margins.right());
    }

    void DialogButton::paintEvent(QPaintEvent * _e)
    {
        QPainter p(this);
        const auto borderWidth = Utils::scale_value(1);
        QRect insideRect = rect().adjusted(borderWidth, borderWidth, -borderWidth, -borderWidth);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(hovered_ ? borderColorHover_ : (pressed_ ? borderColorPress_ : borderColor_));
        p.setBrush(hovered_ ? bgColorHover_ : (pressed_ ? bgColorPress_ : bgColor_));

        const auto radius = height() / 2;
        p.drawRoundedRect(insideRect, radius, radius);

        p.setPen(hovered_ ? textColorHover_ : (pressed_ ? textColorPress_ : textColor_));
        p.drawText(rect(), Qt::AlignCenter, text());
    }

    void DialogButton::leaveEvent(QEvent * _e)
    {
        if (hasFocus())
            return;

        hovered_ = false;
        update();
        updateTextColor();

        QPushButton::leaveEvent(_e);
    }

    void DialogButton::enterEvent(QEvent * _e)
    {
        hovered_ = true;
        update();
        updateTextColor();

        QPushButton::enterEvent(_e);
    }

    void DialogButton::mousePressEvent(QMouseEvent * _e)
    {
        pressed_ = true;

        if (!isEnabled())
            return;

        update();
        updateTextColor();

        QPushButton::mousePressEvent(_e);
    }

    void DialogButton::mouseReleaseEvent(QMouseEvent * _e)
    {
        pressed_ = false;

        if (!isEnabled())
            return;

        update();
        updateTextColor();

        QPushButton::mouseReleaseEvent(_e);
    }

    void DialogButton::focusInEvent(QFocusEvent *_e)
    {
        hovered_ = true;

        if (!isEnabled())
            return;

        update();
        updateTextColor();

        QPushButton::focusInEvent(_e);
    }

    void DialogButton::focusOutEvent(QFocusEvent *_e)
    {
        hovered_ = false;

        if (!isEnabled())
            return;

        update();
        updateTextColor();

        QPushButton::focusOutEvent(_e);
    }

    void DialogButton::keyPressEvent(QKeyEvent *_e)
    {
        if (role_ == DialogButtonRole::DISABLED)
            return;

        if (_e->key() == Qt::Key_Escape || _e->key() == Qt::Key_Space
            || _e->key() == Qt::Key_Return || _e->key() == Qt::Key_Enter)
        {
            pressed_ = true;
            update();
            updateTextColor();
        }
        else
        {
            _e->accept();
            return;
        }

        QPushButton::keyPressEvent(_e);
    }

    void DialogButton::setBorderColor()
    {
        switch (role_)
        {
        case DialogButtonRole::CONFIRM:
        case DialogButtonRole::RESTART:
            borderColor_ = StyleColor(Styling::StyleVariable::PRIMARY);
            borderColorHover_ = StyleColor(Styling::StyleVariable::PRIMARY_HOVER);
            borderColorPress_ = StyleColor(Styling::StyleVariable::PRIMARY_ACTIVE);
            break;

        case DialogButtonRole::CONFIRM_DELETE:
            borderColor_ = StyleColor(Styling::StyleVariable::SECONDARY_ATTENTION);
            borderColorHover_ = StyleColor(Styling::StyleVariable::SECONDARY_ATTENTION);
            borderColorPress_ = StyleColor(Styling::StyleVariable::SECONDARY_ATTENTION);
            break;

        case DialogButtonRole::DISABLED:
            borderColor_ = StyleColor(Styling::StyleVariable::BASE_SECONDARY);
            borderColorHover_ = StyleColor(Styling::StyleVariable::BASE_SECONDARY);
            borderColorPress_ = StyleColor(Styling::StyleVariable::BASE_SECONDARY);
            break;

        case DialogButtonRole::CANCEL:
            borderColor_ = StyleColor(Styling::StyleVariable::BASE_SECONDARY);
            borderColorHover_ = StyleColor(Styling::StyleVariable::BASE_SECONDARY_HOVER);
            borderColorPress_ = StyleColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
            break;

        default:
            break;
        }
    }

    void DialogButton::setBackgroundColor()
    {
        switch (role_)
        {
        case DialogButtonRole::CONFIRM:
            bgColor_ = StyleColor(Styling::StyleVariable::PRIMARY);
            bgColorHover_ = StyleColor(Styling::StyleVariable::PRIMARY_HOVER);
            bgColorPress_ = StyleColor(Styling::StyleVariable::PRIMARY_ACTIVE);
            break;

        case DialogButtonRole::CONFIRM_DELETE:
            bgColor_ = StyleColor(Styling::StyleVariable::SECONDARY_ATTENTION);
            bgColorHover_ = StyleColor(Styling::StyleVariable::SECONDARY_ATTENTION);
            bgColorPress_ = StyleColor(Styling::StyleVariable::SECONDARY_ATTENTION);
            break;

        case DialogButtonRole::DISABLED:
            bgColor_ = StyleColor(Styling::StyleVariable::BASE_SECONDARY);
            bgColorHover_ = StyleColor(Styling::StyleVariable::BASE_SECONDARY);
            bgColorPress_ = StyleColor(Styling::StyleVariable::BASE_SECONDARY);
            break;

        case DialogButtonRole::CANCEL:
        case DialogButtonRole::RESTART:
            break;

        default:
            break;
        }
    }

    void DialogButton::setTextColor()
    {
        switch (role_)
        {
        case DialogButtonRole::CONFIRM:
        case DialogButtonRole::CONFIRM_DELETE:
        case DialogButtonRole::DISABLED:
            textColor_ = StyleColor(Styling::StyleVariable::BASE_GLOBALWHITE);
            textColorHover_ = StyleColor(Styling::StyleVariable::BASE_GLOBALWHITE);
            textColorPress_ = StyleColor(Styling::StyleVariable::BASE_GLOBALWHITE);
            break;
        case DialogButtonRole::CANCEL:
            textColor_ = StyleColor(Styling::StyleVariable::BASE_SECONDARY);
            textColorHover_ = StyleColor(Styling::StyleVariable::BASE_SECONDARY_HOVER);
            textColorPress_ = StyleColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
            break;

        case DialogButtonRole::RESTART:
            textColor_ = StyleColor(Styling::StyleVariable::TEXT_PRIMARY);
            textColorHover_ = StyleColor(Styling::StyleVariable::TEXT_PRIMARY);
            textColorPress_ = StyleColor(Styling::StyleVariable::TEXT_PRIMARY);
            break;

        default:
            break;
        }
    }

    void DialogButton::updateTextColor()
    {
        QPalette palette = this->palette();
        palette.setColor(QPalette::ButtonText, hovered_ ? textColorHover_ : (pressed_ ? textColorPress_ : textColor_));
        setPalette(palette);
    }

    void DialogButton::setBackgroundColor(QColor _normal, QColor _hover, QColor _pressed)
    {
        bgColor_ = _normal;
        bgColorHover_ = _hover;
        bgColorPress_ = _pressed;
    }

    void DialogButton::setBorderColor(QColor _normal, QColor _hover, QColor _pressed)
    {
        borderColor_ = _normal;
        borderColorHover_ = _hover;
        borderColorPress_ = _pressed;
    }

    void DialogButton::setTextColor(QColor _normal, QColor _hover, QColor _pressed)
    {
        textColor_ = _normal;
        textColorHover_ = _hover;
        textColorPress_ = _pressed;
    }
}
