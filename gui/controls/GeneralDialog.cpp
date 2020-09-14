#include "stdafx.h"

#include "GeneralDialog.h"
#include "TextEmojiWidget.h"
#include "TextEditEx.h"
#include "SemitransparentWindowAnimated.h"
#include "../fonts.h"
#include "../gui_settings.h"
#include "../main_window/MainWindow.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "styles/ThemeParameters.h"
#include "../controls/DialogButton.h"
#include "../controls/TextUnit.h"

namespace
{
    constexpr std::chrono::milliseconds animationDuration = std::chrono::milliseconds(100);
    constexpr auto defaultDialogWidth()
    {
        return 360;
    }

    int getMargin()
    {
        return Utils::scale_value(16);
    }

    int getTopMargin()
    {
        return Utils::scale_value(8);
    }

    int getExtendedVerMargin()
    {
        return Utils::scale_value(20);
    }

    int getIconSize()
    {
        return Utils::scale_value(20);
    }

    int getOptionHeight()
    {
        return Utils::scale_value(44);
    }
}

namespace Ui
{
    bool GeneralDialog::inExec_ = false;

    GeneralDialog::GeneralDialog(QWidget* _mainWidget, QWidget* _parent, bool _ignoreKeyPressEvents/* = false*/, bool _fixed_size/* = true*/, bool _rejectable/* = true*/, bool _withSemiwindow/*= true*/, const Options& _options)
        : QDialog(nullptr)
        , mainWidget_(_mainWidget)
        , nextButton_(nullptr)
        , semiWindow_(new SemitransparentWindowAnimated(_parent, animationDuration.count()))
        , headerLabelHost_(nullptr)
        , areaWidget_(nullptr)
        , ignoreKeyPressEvents_(_ignoreKeyPressEvents)
        , shadow_(true)
        , leftButtonDisableOnClicked_(false)
        , rightButtonDisableOnClicked_(false)
        , rejectable_(_rejectable)
        , withSemiwindow_(_withSemiwindow)
        , options_(_options)
    {
        setParent(semiWindow_);

        semiWindow_->setCloseWindowInfo({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });
        semiWindow_->hide();

        shadowHost_ = new QWidget(this);
        shadowHost_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

        mainHost_ = new QWidget(this);
        Utils::setDefaultBackground(mainHost_);
        Testing::setAccessibleName(mainHost_, qsl("AS GeneralPopup"));

        if (_fixed_size)
        {
            if (options_.preferredSize_.isNull())
                mainHost_->setFixedWidth(Utils::scale_value(defaultDialogWidth()));
            else
                mainHost_->setFixedWidth(options_.preferredSize_.width());

            shadowHost_->setFixedWidth(mainHost_->width() + Ui::get_gui_settings()->get_shadow_width() * 2);
        }
        else
        {
            mainHost_->setMaximumSize(Utils::GetMainRect().size() - 2 * Utils::scale_value(QSize(8, 8)));
            shadowHost_->setMaximumWidth(mainHost_->maximumWidth() + Ui::get_gui_settings()->get_shadow_width() * 2);
        }

        auto globalLayout = Utils::emptyVLayout(mainHost_);
        globalLayout->setAlignment(Qt::AlignTop);

        headerLabelHost_ = new QWidget(mainHost_);
        headerLabelHost_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        headerLabelHost_->setVisible(false);
        Testing::setAccessibleName(headerLabelHost_, qsl("AS GeneralPopup headerLabel"));
        globalLayout->addWidget(headerLabelHost_);

        errorHost_ = new QWidget(mainHost_);
        errorHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        Utils::ApplyStyle(errorHost_, qsl("height: 1dip;"));
        errorHost_->setVisible(false);
        Testing::setAccessibleName(errorHost_, qsl("AS GeneralPopup errorLabel"));
        globalLayout->addWidget(errorHost_);

        textHost_ = new QWidget(mainHost_);
        textHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        Utils::ApplyStyle(textHost_, qsl("height: 1dip;"));
        textHost_->setVisible(false);
        Testing::setAccessibleName(textHost_, qsl("AS GeneralPopup textLabel"));
        globalLayout->addWidget(textHost_);

        if (mainWidget_)
        {
            if constexpr (platform::is_apple())
                globalLayout->addSpacing(Utils::scale_value(5));
            globalLayout->addWidget(mainWidget_);
        }

        areaWidget_ = new QWidget(mainHost_);
        areaWidget_->setVisible(false);
        Testing::setAccessibleName(areaWidget_, qsl("AS GeneralPopup areaWidget"));
        globalLayout->addWidget(areaWidget_);

        bottomWidget_ = new QWidget(mainHost_);
        bottomWidget_->setVisible(false);
        bottomWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        Utils::setDefaultBackground(bottomWidget_);
        Testing::setAccessibleName(bottomWidget_, qsl("AS GeneralPopup bottomWidget"));
        globalLayout->addWidget(bottomWidget_);

        setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowSystemMenuHint | Qt::SubWindow);
        setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);

        if (rejectable_)
        {
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnySemitransparentWindow, this, &GeneralDialog::rejectDialog);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupWindow, this, &GeneralDialog::rejectDialog);
        }

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mainWindowResized, this, &GeneralDialog::updateSize);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::acceptGeneralDialog, this, &GeneralDialog::acceptDialog);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationLocked, this, &GeneralDialog::close);

        auto mainLayout = Utils::emptyVLayout(this);
        mainLayout->addWidget(shadowHost_);

        auto shadowLayout = Utils::emptyVLayout(shadowHost_);
        shadowLayout->addWidget(mainHost_);

        auto parentLayout = Utils::emptyVLayout(parentWidget());
        parentLayout->setAlignment(Qt::AlignHCenter);
        parentLayout->addWidget(this);

        setAttribute(Qt::WA_QuitOnClose, false);
        installEventFilter(this);

        if (auto prevFocused = qobject_cast<QWidget*>(qApp->focusObject()))
        {
            connect(this, &GeneralDialog::finished, prevFocused, [prevFocused]()
            {
                prevFocused->setFocus();
            });
        }
        setFocus();
    }

    void GeneralDialog::rejectDialog(const Utils::CloseWindowInfo& _info)
    {
        if (std::any_of(
                options_.ignoreRejectDlgPairs_.begin(),
                options_.ignoreRejectDlgPairs_.end(),
                [&_info](const auto& _p) { return _p.first == _info.initiator_ && _p.second == _info.reason_; })
            )
            return;

        rejectDialogInternal();
    }

    void GeneralDialog::rejectDialogInternal()
    {
        if (!rejectable_)
            return;

        if (withSemiwindow_)
            semiWindow_->Hide();

        QDialog::reject();
    }

    void GeneralDialog::acceptDialog()
    {
        if (withSemiwindow_)
            semiWindow_->Hide();

        QDialog::accept();
    }

    void GeneralDialog::done(int r)
    {
        window()->setFocus();
        QDialog::done(r);
    }

    void GeneralDialog::addLabel(const QString& _text, Qt::Alignment _alignment, int _maxLinesNumber)
    {
        headerLabelHost_->setVisible(true);
        auto hostLayout = Utils::emptyHLayout(headerLabelHost_);

        auto text = Ui::TextRendering::MakeTextUnit(_text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
        text->init(Fonts::appFontScaled(22), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, _maxLinesNumber, TextRendering::LineBreakType::PREFER_SPACES);

        const auto maxWidth = mainHost_->maximumWidth() - 2 * getMargin();
        auto label = new TextUnitLabel(textHost_, std::move(text), Ui::TextRendering::VerPosition::TOP, maxWidth, true);
        label->setMaximumWidth(maxWidth);
        label->sizeHint();

        const auto leftMargin = (_alignment & Qt::AlignHCenter) ? (maxWidth - label->width()) / 2 + getMargin() : getMargin();
        const auto topMargin = (_alignment & Qt::AlignVCenter) ? getExtendedVerMargin() : getTopMargin();
        hostLayout->setContentsMargins(leftMargin, topMargin, 0, 0);
        hostLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        Testing::setAccessibleName(label, qsl("AS GeneralPopup label"));
        hostLayout->addWidget(label);
    }

    void GeneralDialog::addText(const QString& _messageText, int _upperMarginPx)
    {
        addText(_messageText, _upperMarginPx, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    }

    void GeneralDialog::addText(const QString& _messageText, int _upperMarginPx, const QFont& _font, const QColor& _color)
    {
        auto text = Ui::TextRendering::MakeTextUnit(_messageText, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        text->init(_font, _color);

        const auto maxWidth = mainHost_->maximumWidth() - 2 * getMargin();
        auto label = new TextUnitLabel(textHost_, std::move(text), Ui::TextRendering::VerPosition::TOP, maxWidth);
        label->setFixedWidth(maxWidth);
        label->sizeHint();
        Testing::setAccessibleName(label, qsl("AS GeneralPopup textLabel"));

        auto textLayout = Utils::emptyHLayout(textHost_);
        textLayout->setContentsMargins(getMargin(), _upperMarginPx, getMargin(), 0);
        textLayout->addWidget(label);
        textHost_->setVisible(true);
    }


    DialogButton* GeneralDialog::addAcceptButton(const QString& _buttonText, const bool _isEnabled)
    {
        {
            nextButton_ =  new DialogButton(bottomWidget_, _buttonText, _isEnabled ? DialogButtonRole::CONFIRM : DialogButtonRole::DISABLED);
            nextButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            nextButton_->adjustSize();
            setButtonActive(_isEnabled);

            connect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::accept, Qt::QueuedConnection);

            Testing::setAccessibleName(nextButton_, qsl("AS GeneralPopup nextButton"));

            auto bottomLayout = getBottomLayout();
            bottomLayout->addWidget(nextButton_);
        }

        bottomWidget_->setVisible(true);

        return nextButton_;
    }

    DialogButton* GeneralDialog::addCancelButton(const QString& _buttonText, const bool _setActive)
    {
        {
            nextButton_ = new DialogButton(bottomWidget_, _buttonText, DialogButtonRole::CANCEL);
            nextButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            nextButton_->adjustSize();
            setButtonActive(_setActive);

            connect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::reject, Qt::QueuedConnection);

            Testing::setAccessibleName(nextButton_, qsl("AS GeneralPopup cancelButton"));

            auto bottomLayout = getBottomLayout();
            bottomLayout->addWidget(nextButton_);
        }

        bottomWidget_->setVisible(true);

        return nextButton_;
    }

    QPair<DialogButton*, DialogButton*> GeneralDialog::addButtonsPair(const QString& _buttonTextLeft, const QString& _buttonTextRight, bool _isActive, bool _rejectable, bool _acceptable, QWidget* _area)
    {
        QPair<DialogButton*, DialogButton*> result;
        {
            auto cancelButton = new DialogButton(bottomWidget_, _buttonTextLeft, DialogButtonRole::CANCEL);
            cancelButton->setObjectName(qsl("left_button"));
            cancelButton->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            connect(cancelButton, &DialogButton::clicked, this, &GeneralDialog::leftButtonClick, Qt::QueuedConnection);
            if (_rejectable)
                connect(cancelButton, &DialogButton::clicked, this, &GeneralDialog::reject, Qt::QueuedConnection);
            Testing::setAccessibleName(cancelButton, qsl("AS GeneralPopup cancelButton"));

            nextButton_ = new DialogButton(bottomWidget_, _buttonTextRight, DialogButtonRole::CONFIRM);
            setButtonActive(_isActive);
            nextButton_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            connect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::rightButtonClick, Qt::QueuedConnection);
            if (_acceptable)
                connect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::accept, Qt::QueuedConnection);
            Testing::setAccessibleName(nextButton_, qsl("AS GeneralPopup nextButton"));

            auto bottomLayout = getBottomLayout();
            bottomLayout->addWidget(cancelButton);
            bottomLayout->addSpacing(getMargin());
            bottomLayout->addWidget(nextButton_);

            result = { nextButton_, cancelButton };
        }

        if (_area)
        {
            auto v = Utils::emptyVLayout(areaWidget_);
            Testing::setAccessibleName(_area, qsl("AS GeneralPopup area"));
            v->addWidget(_area);
            areaWidget_->setVisible(true);
        }

        bottomWidget_->setVisible(true);

        return result;
    }

    void GeneralDialog::setButtonsAreaMargins(const QMargins& _margins)
    {
        getBottomLayout()->setContentsMargins(_margins);
    }

    DialogButton* GeneralDialog::takeAcceptButton()
    {
        if (nextButton_)
            QObject::disconnect(nextButton_, &DialogButton::clicked, this, &GeneralDialog::accept);

        return nextButton_;
    }

    void GeneralDialog::setAcceptButtonText(const QString& _text)
    {
        if (!nextButton_)
            return;

        nextButton_->setText(_text);
    }

    GeneralDialog::~GeneralDialog()
    {
        semiWindow_->close();
    }

    bool GeneralDialog::showInCenter()
    {
        if (shadow_)
        {
            shadow_ = false;
            Utils::addShadowToWindow(shadowHost_, true);
        }

        if (!semiWindow_->isVisible() && withSemiwindow_)
            semiWindow_->Show();

        show();
        inExec_ = true;
        const auto guard = QPointer(this);
        const auto result = (exec() == QDialog::Accepted);
        if (!guard)
            return false;
        inExec_ = false;
        close();

        if constexpr (platform::is_apple())
            semiWindow_->parentWidget()->activateWindow();
        if (semiWindow_->isVisible())
            semiWindow_->Hide();
        return result;
    }

    QWidget* GeneralDialog::getMainHost()
    {
        return mainHost_;
    }

    void GeneralDialog::setButtonActive(bool _active)
    {
        if (!nextButton_)
            return;

        nextButton_->setEnabled(_active);
    }

    QHBoxLayout* GeneralDialog::getBottomLayout()
    {
        assert(bottomWidget_);
        if (bottomWidget_->layout())
        {
            assert(qobject_cast<QHBoxLayout*>(bottomWidget_->layout()));
            return qobject_cast<QHBoxLayout*>(bottomWidget_->layout());
        }

        auto bottomLayout = Utils::emptyHLayout(bottomWidget_);
        bottomLayout->setContentsMargins(getMargin(), getMargin(), getMargin(), getMargin());
        bottomLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
        return bottomLayout;
    }

    void GeneralDialog::mousePressEvent(QMouseEvent* _e)
    {
        QDialog::mousePressEvent(_e);

        if (rejectable_ && !mainHost_->rect().contains(mainHost_->mapFrom(this, _e->pos())))
            close();
        else
            _e->accept();
    }

    void GeneralDialog::keyPressEvent(QKeyEvent* _e)
    {
        if (ignoredKeys_.find(static_cast<Qt::Key>(_e->key())) != ignoredKeys_.end())
            return;

        if (_e->key() == Qt::Key_Escape)
        {
            close();
            return;
        }
        else if (_e->key() == Qt::Key_Return || _e->key() == Qt::Key_Enter)
        {
            if (!ignoreKeyPressEvents_ && (!nextButton_ || nextButton_->isEnabled()))
                accept();
            else
                _e->ignore();
            return;
        }

        QDialog::keyPressEvent(_e);
    }

    bool GeneralDialog::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_obj == this)
        {
            if (_event->type() == QEvent::ShortcutOverride || _event->type() == QEvent::KeyPress)
            {
                auto keyEvent = static_cast<QKeyEvent*>(_event);
                if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
                {
                    keyEvent->accept();
                    return true;
                }
            }
        }

        return false;
    }

    void GeneralDialog::showEvent(QShowEvent *event)
    {
        QDialog::showEvent(event);
        Q_EMIT shown(this);
    }

    void GeneralDialog::hideEvent(QHideEvent *event)
    {
        Q_EMIT hidden(this);
        QDialog::hideEvent(event);
    }

    void GeneralDialog::moveEvent(QMoveEvent *_event)
    {
        QDialog::moveEvent(_event);
        Q_EMIT moved(this);
    }

    void GeneralDialog::resizeEvent(QResizeEvent* _event)
    {
        QDialog::resizeEvent(_event);
        updateSize();

        Q_EMIT resized(this);
    }

    void GeneralDialog::addError(const QString& _messageText)
    {
        errorHost_->setVisible(true);
        errorHost_->setContentsMargins(0, 0, 0, 0);
        errorHost_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);

        auto textLayout = Utils::emptyVLayout(errorHost_);

        auto upperSpacer = new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Minimum);
        textLayout->addSpacerItem(upperSpacer);

        const QString backgroundStyle = qsl("background-color: #fbdbd9; ");
        const QString labelStyle = u"QWidget { " % backgroundStyle % u"border: none; padding-left: 16dip; padding-right: 16dip; padding-top: 0dip; padding-bottom: 0dip; }";

        auto upperSpacerRedUp = new QLabel();
        upperSpacerRedUp->setFixedHeight(Utils::scale_value(16));
        Utils::ApplyStyle(upperSpacerRedUp, backgroundStyle);
        Testing::setAccessibleName(upperSpacerRedUp, qsl("AS GeneralPopup upperSpacerRedUp"));
        textLayout->addWidget(upperSpacerRedUp);

        auto errorLabel = new Ui::TextEditEx(errorHost_, Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION), true, true);
        errorLabel->setContentsMargins(0, 0, 0, 0);
        errorLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        errorLabel->setPlaceholderText(QString());
        errorLabel->setAutoFillBackground(false);
        errorLabel->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        errorLabel->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::ApplyStyle(errorLabel, labelStyle);

        errorLabel->setText(QT_TRANSLATE_NOOP("popup_window", "Unfortunately, an error occurred:"));
        Testing::setAccessibleName(errorLabel, qsl("AS GeneralPopup errorLabel"));
        textLayout->addWidget(errorLabel);

        auto errorText = new Ui::TextEditEx(errorHost_, Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), true, true);
        errorText->setContentsMargins(0, 0, 0, 0);
        errorText->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        errorText->setPlaceholderText(QString());
        errorText->setAutoFillBackground(false);
        errorText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        errorText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::ApplyStyle(errorText, labelStyle);

        errorText->setText(_messageText);
        Testing::setAccessibleName(errorText, qsl("AS GeneralPopup errorText"));
        textLayout->addWidget(errorText);

        auto upperSpacerRedBottom = new QLabel();
        Utils::ApplyStyle(upperSpacerRedBottom, backgroundStyle);
        upperSpacerRedBottom->setFixedHeight(Utils::scale_value(16));
        Testing::setAccessibleName(upperSpacerRedBottom, qsl("AS GeneralPopup upperSpacerRedBottom"));
        textLayout->addWidget(upperSpacerRedBottom);

        auto upperSpacer2 = new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Minimum);
        textLayout->addSpacerItem(upperSpacer2);

    }

    void GeneralDialog::leftButtonClick()
    {
        if (auto leftButton = bottomWidget_->findChild<DialogButton*>(qsl("left_button")))
            leftButton->setEnabled(!leftButtonDisableOnClicked_);
        Q_EMIT leftButtonClicked();
    }

    void GeneralDialog::rightButtonClick()
    {
        nextButton_->setEnabled(!rightButtonDisableOnClicked_);
        Q_EMIT rightButtonClicked();
    }

    void GeneralDialog::updateSize()
    {
        if (semiWindow_)
            semiWindow_->updateSize();
    }

    void GeneralDialog::setIgnoreClicksInSemiWindow(bool _ignore)
    {
        if (semiWindow_)
            semiWindow_->setForceIgnoreClicks(_ignore);
    }

    void GeneralDialog::setIgnoredKeys(const GeneralDialog::KeysContainer &_keys)
    {
        ignoredKeys_ = _keys;
    }

    bool GeneralDialog::isActive()
    {
        return inExec_;
    }

    int GeneralDialog::getHeaderHeight() const
    {
        return headerLabelHost_ ? headerLabelHost_->sizeHint().height() : 0;
    }

    void GeneralDialog::setTransparentBackground(bool _enable)
    {
        if (_enable)
            Utils::updateBgColor(mainHost_, Qt::transparent);
        else
            Utils::setDefaultBackground(mainHost_);
    }

    TwoOptionsWidget::TwoOptionsWidget(QWidget* _parent, const QString& _firstOptionIcon, const QString& _firstOption, const QString& _secondOptionIcon, const QString& _secondOption)
        : QWidget(_parent)
        , firstHovered_(false)
        , firstSelected_(false)
        , secondHovered_(false)
        , secondSelected_(false)
    {
        firstOption_ = TextRendering::MakeTextUnit(_firstOption, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        firstOption_->init(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        firstOptionIcon_ = Utils::renderSvg(_firstOptionIcon, QSize(getIconSize(), getIconSize()), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));

        secondOption_ = TextRendering::MakeTextUnit(_secondOption, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        secondOption_->init(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        secondOptionIcon_ = Utils::renderSvg(_secondOptionIcon, QSize(getIconSize(), getIconSize()), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));

        setFixedHeight(getOptionHeight() * 2 + getMargin());
        setMouseTracking(true);
    }

    bool TwoOptionsWidget::isFirstSelected() const
    {
        return firstSelected_;
    }

    bool TwoOptionsWidget::isSecondSelected() const
    {
        return secondSelected_;
    }

    void TwoOptionsWidget::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);

        if (firstHovered_)
            p.fillRect(0, getMargin(), width(), getOptionHeight(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
        if (secondHovered_)
            p.fillRect(0, getOptionHeight() + getMargin(), width(), getOptionHeight(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

        p.drawPixmap(getMargin(), getOptionHeight() / 2 - getIconSize() / 2 + getMargin(), firstOptionIcon_);
        firstOption_->setOffsets(getMargin() * 2 + getIconSize(), getOptionHeight() / 2 + getMargin());
        firstOption_->draw(p, TextRendering::VerPosition::MIDDLE);

        p.drawPixmap(getMargin(), (getOptionHeight() / 2) * 3 - getIconSize() / 2 + getMargin(), secondOptionIcon_);
        secondOption_->setOffsets(getMargin() * 2 + getIconSize(), (getOptionHeight() / 2) * 3 + getMargin());
        secondOption_->draw(p, TextRendering::VerPosition::MIDDLE);
    }

    void TwoOptionsWidget::resizeEvent(QResizeEvent* _event)
    {
        firstOption_->elide(width() - getMargin() * 3 - getIconSize());
        secondOption_->elide(width() - getMargin() * 3 - getIconSize());

        QWidget::resizeEvent(_event);
    }

    void TwoOptionsWidget::mouseMoveEvent(QMouseEvent* _event)
    {
        firstHovered_ = false;
        secondHovered_ = false;

        if (QRect(0, getMargin(), width(), getOptionHeight()).contains(_event->pos()))
            firstHovered_ = true;
        else if (QRect(0, getOptionHeight() + getMargin(), width(), getOptionHeight()).contains(_event->pos()))
            secondHovered_ = true;

        setCursor((firstHovered_ || secondHovered_) ? Qt::PointingHandCursor : Qt::ArrowCursor);

        update();
        QWidget::mouseMoveEvent(_event);
    }

    void TwoOptionsWidget::mousePressEvent(QMouseEvent* _event)
    {
        pos_ = _event->pos();
        QWidget::mousePressEvent(_event);
    }

    void TwoOptionsWidget::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (Utils::clicked(pos_, _event->pos()))
        {
            if (QRect(0, getMargin(), width(), getOptionHeight()).contains(_event->pos()))
                firstSelected_ = true;
            else if (QRect(0, getOptionHeight() + getMargin(), width(), getOptionHeight()).contains(_event->pos()))
                secondSelected_ = true;

            Q_EMIT Utils::InterConnector::instance().acceptGeneralDialog();
        }
        QWidget::mouseReleaseEvent(_event);
    }

    void TwoOptionsWidget::leaveEvent(QEvent* _event)
    {
        firstHovered_ = false;
        secondHovered_ = false;
        setCursor(Qt::ArrowCursor);
        update();
        QWidget::leaveEvent(_event);
    }
}
