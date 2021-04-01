#include "stdafx.h"

#include "../controls/CustomButton.h"
#include "../utils/utils.h"

#include "../controls/TextUnit.h"

#include "../controls/TooltipWidget.h"
#include "../fonts.h"
#include "Drawable.h"
#include "styles/ThemeParameters.h"

#ifdef __APPLE__
#include "utils/macos/mac_support.h"
#endif

#include "GalleryFrame.h"

namespace
{
    constexpr QStringView imagePath = u":/gallery/glr_";
    const auto backgroundOpacity = 0.92;
    const auto backgroundColor = QColor(u"#1E1E1E");

    auto getMenuBorderRadius() noexcept
    {
        return Utils::scale_value(8);
    }
}

namespace Previewer
{

using namespace Ui;

enum ControlType : short
{
    ZoomOutButton,
    ZoomInButton,
    PrevButton,
    NextButton,
    CloseButton,
    MenuButton,
    SaveButton,
    CounterLabel,
    AuthorLabel,
    DateLabel
};

auto getTopMargin() { return Utils::scale_value(70); }

int topMargin = getTopMargin();

int max_height = Utils::scale_value(268);

enum Margin : int
{
    _4px = 4,
    _8px = 8,
    _12px = 12,
    _16px = 16,
    _20px = 20,
    _24px = 24,
    _32px = 32
};

CaptionArea::CaptionArea(QWidget* _parent)
    : QScrollArea(_parent)
    , expanded_(false)
    , totalWidth_(0)
    , anim_(new QVariantAnimation(this))
{
    textWidget_ = new Ui::TextWidget(this, QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
    textWidget_->init(Fonts::appFontScaled(15, Fonts::FontWeight::Normal), Qt::white);

    textPreview_ = TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    textPreview_->init(Fonts::appFontScaled(15, Fonts::FontWeight::Normal), Qt::white, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 2, TextRendering::LineBreakType::PREFER_SPACES);

    more_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("previewer", "more"));
    more_->init(Fonts::appFontScaled(15, Fonts::FontWeight::Normal), Qt::white);
    more_->setUnderline(true);
    more_->evaluateDesiredSize();

    setStyleSheet(qsl("background: transparent;"));
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setWidget(textWidget_);
    setWidgetResizable(true);
    setFocusPolicy(Qt::NoFocus);
    setFrameShape(QFrame::NoFrame);
    Utils::ApplyStyle(verticalScrollBar(), qsl("\
            QScrollBar:vertical\
            {\
                width: 4dip;\
                margin-top: 0px;\
                margin-bottom: 0px;\
                margin-right: 0px;\
            }"));


    setExpanded(false);
    setMouseTracking(true);
    setAutoFillBackground(true);

    anim_->setDuration(300);
    anim_->setEasingCurve(QEasingCurve::OutExpo);
}

void CaptionArea::setCaption(const QString& _caption, int _totalWidth)
{
    totalWidth_ = _totalWidth;
    textWidget_->setText(_caption);
    textPreview_->setText(_caption);
    setExpanded(false);
}

void CaptionArea::updateSize()
{
    static auto max_width = Utils::scale_value(684);

    if (expanded_)
    {
        textWidget_->setMaxWidthAndResize(max_width - Utils::scale_value(8));
    }
    else
    {
        textPreview_->setLastLineWidth(max_width - more_->cachedSize().width());
        setFixedSize(max_width, textPreview_->getHeight(max_width));
        auto xOffset = textPreview_->desiredWidth() < max_width ? (width() / 2 - textPreview_->desiredWidth() / 2) : 0;
        textPreview_->setOffsets(xOffset, 0);
        more_->setOffsets(max_width - more_->cachedSize().width(), height() - more_->cachedSize().height());
    }
}

void CaptionArea::setExpanded(bool _expanded)
{
    expanded_ = _expanded;
    textWidget_->setVisible(_expanded);

    updateSize();

    auto addSpace = height() + Utils::scale_value(Margin::_32px);
    topMargin = expanded_ ? addSpace : getTopMargin();

    move(totalWidth_ / 2 - width() / 2, expanded_ ? Utils::scale_value(Margin::_16px) : topMargin - height() - Utils::scale_value(_16px));
    if (expanded_)
    {
        if (anim_->state() != QAbstractAnimation::State::Running)
        {
            const auto endHeight = std::min(max_height, textWidget_->height());
            const auto startHeight = height();
            const auto endSpace = endHeight - getTopMargin() + Utils::scale_value(Margin::_32px);

            anim_->stop();
            anim_->setStartValue(0);
            anim_->setEndValue(endSpace);
            anim_->disconnect(this);
            connect(anim_, &QVariantAnimation::valueChanged, this, [this, endSpace, endHeight, startHeight](const QVariant& value)
            {
                Q_EMIT needHeight(value.toInt());
                setFixedHeight(startHeight + (endHeight - startHeight) * (value.toDouble() / endSpace));
            });
            anim_->start();
        }
    }
    else
    {
        Q_EMIT needHeight(0);
    }

    setVerticalScrollBarPolicy((expanded_ && textWidget_->height() > max_height) ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
    update();
}

bool CaptionArea::isExpanded() const
{
    return expanded_;
}

void CaptionArea::paintEvent(QPaintEvent* _e)
{
    QPainter p(viewport());
    if (!expanded_)
    {
        textPreview_->draw(p);
        if (textPreview_->isElided())
            more_->draw(p);
        return;
    }

    QScrollArea::paintEvent(_e);
}

void CaptionArea::mouseMoveEvent(QMouseEvent* _e)
{
    if (!expanded_ && textPreview_->isElided())
    {
        setCursor(more_->contains(_e->pos()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }

    QScrollArea::mouseMoveEvent(_e);
}

void CaptionArea::enterEvent(QEvent* _e)
{
    setVerticalScrollBarPolicy((expanded_ && textWidget_->height() > max_height) ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
    update();

    QScrollArea::enterEvent(_e);
}

void CaptionArea::leaveEvent(QEvent* _e)
{
    setCursor(Qt::ArrowCursor);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    update();

    QScrollArea::leaveEvent(_e);
}

void CaptionArea::mousePressEvent(QMouseEvent* _e)
{
    clicked_ = _e->pos();
    QScrollArea::mousePressEvent(_e);
}

void CaptionArea::mouseReleaseEvent(QMouseEvent* _e)
{
    if (Utils::clicked(clicked_, _e->pos()))
    {
        if (!expanded_ && textPreview_->isElided() && more_->contains(_e->pos()))
            setExpanded(!expanded_);
    }

    QScrollArea::mouseReleaseEvent(_e);
}

class GalleryFrame_p
{
public:

    enum ButtonState
    {
        Default     = 0x1,
        Disabled    = 0x4,
        Hover       = 0x10,
        Pressed     = 0x20
    };

    QString getImagePath(QStringView name, ButtonState state) const
    {
        switch (state)
        {
            case Default:   return imagePath % name % u"_icon";
            case Disabled:  return imagePath % name % u"_icon" % u"_disabled";
            case Hover:     return imagePath % name % u"_icon" % u"_hover";
            case Pressed:   return imagePath % name % u"_icon" % u"_active";
        }
        im_assert(!"invalid kind!");
        return QString();
    }

    auto objectSize(ControlType _type) const
    {
        switch (_type)
        {
            case ZoomInButton:
            case ZoomOutButton:
                return Utils::scale_value(QSize(44, 40));

            case PrevButton:
            case NextButton:
            case CloseButton:
            case MenuButton:
            case SaveButton:
                return Utils::scale_value(QSize(40, 40));

            case CounterLabel:
                return Utils::scale_value(QSize(180, 36));

            case AuthorLabel:
            case DateLabel:
                return Utils::scale_value(QSize(137, 20));

            default:
                return QSize();
        }
    }

    auto solidRectHeight() const { return Utils::scale_value(40); }

    ButtonAccessible* addButton(ControlType _type, QStringView _iconName, QWidget* parent)
    {
        auto button = std::make_unique<ButtonAccessible>(parent);
        const auto size = objectSize(_type);
        button->setDefaultPixmap(Utils::renderSvg(getImagePath(_iconName, GalleryFrame_p::Default), size));
        button->setHoveredPixmap(Utils::renderSvg(getImagePath(_iconName, GalleryFrame_p::Hover), size));
        button->setPressedPixmap(Utils::renderSvg(getImagePath(_iconName, GalleryFrame_p::Pressed), size));
        button->setDisabledPixmap(Utils::renderSvg(getImagePath(_iconName, GalleryFrame_p::Disabled), size));

        auto pointer = button.get();
        objects_[_type] = std::move(button);
        return pointer;
    }

    QPoint objectPosition(ControlType _type, int _width, int _height) const
    {
        auto offset = _height - Utils::scale_value(92);
        switch (_type)
        {
            case DateLabel:
                return {objectPosition(AuthorLabel, _width, _height).x(), offset + objectSize(AuthorLabel).height()};
            case AuthorLabel:
                return {objectPosition(ZoomOutButton, _width, _height).x() - Utils::scale_value(Margin::_32px) - objectSize(_type).width(), offset};
            case ZoomOutButton:
                return {objectPosition(ZoomInButton, _width, _height).x() - Utils::scale_value(Margin::_4px) - objectSize(_type).width(), offset};
            case ZoomInButton:
                return {objectPosition(PrevButton, _width, _height).x() - Utils::scale_value(Margin::_32px) - objectSize(_type).width(), offset};
            case PrevButton:
                return {objectPosition(CounterLabel, _width, _height).x() - Utils::scale_value(Margin::_16px) - objectSize(_type).width(), offset};
            case CounterLabel:
                return {(_width - objectSize(_type).width()) / 2, offset};
            case NextButton:
                return {objectPosition(CounterLabel, _width, _height).x() + Utils::scale_value(Margin::_16px) + objectSize(CounterLabel).width(), offset};
            case SaveButton:
                return {objectPosition(NextButton, _width, _height).x() + Utils::scale_value(Margin::_32px) + objectSize(NextButton).width(), offset};
            case MenuButton:
                return {objectPosition(SaveButton, _width, _height).x() + Utils::scale_value(Margin::_12px) + objectSize(SaveButton).width(), offset};
            case CloseButton:
                return {objectPosition(MenuButton, _width, _height).x() + Utils::scale_value(Margin::_32px) + objectSize(MenuButton).width(), offset};
        }
        return QPoint();
    }

    void updateObjectsPositions(int _width, int _height)
    {
        for (auto & [type, object] : objects_)
        {
            object->setRect(QRect(objectPosition(type, _width, _height), objectSize(type)));
        }
    }

    void resetObjects()
    {
        for (auto & [_, object] : objects_)
        {
            object->setHovered(false);
            object->setPressed(false);
        }
    }

    void setCaption(const QString& _caption, int _width)
    {
        caption_->setCaption(_caption, _width);
    }

    void collapseCaption()
    {
        caption_->setExpanded(false);
    }

    bool isCaptionExpanded()
    {
        return caption_->isExpanded();
    }

    std::unordered_map<ControlType, std::unique_ptr<Drawable>> objects_;
    QPointer<CustomMenu> menu_;
    std::unique_ptr<CaptionArea> caption_;
    GalleryFrame::Actions menuActions_;
};

GalleryFrame::GalleryFrame(QWidget *_parent)
    : QWidget(_parent)
    , d(std::make_unique<GalleryFrame_p>())
{
    topMargin = getTopMargin();

    if (auto button = d->addButton(ZoomOutButton, u"zoom_out", this))
        button->setAccessibleName(qsl("AS Preview zoomOut"));

    if (auto button = d->addButton(ZoomInButton, u"zoom_in", this))
        button->setAccessibleName(qsl("AS Preview zoomIn"));

    if (auto button = d->addButton(PrevButton, u"previous", this))
        button->setAccessibleName(qsl("AS Preview previous"));

    if (auto button = d->addButton(NextButton, u"next", this))
        button->setAccessibleName(qsl("AS Preview next"));

    if (auto button = d->addButton(CloseButton, u"close", this))
        button->setAccessibleName(qsl("AS Preview close"));

    if (auto button = d->addButton(MenuButton, u"more", this))
        button->setAccessibleName(qsl("AS Preview more"));

    if (auto button = d->addButton(SaveButton, u"download", this))
        button->setAccessibleName(qsl("AS Preview download"));

    const auto counterFont(Fonts::appFontScaled(20, Fonts::FontWeight::Normal));
    auto counterLabelUnit = TextRendering::MakeTextUnit(QString());
    counterLabelUnit->init(counterFont, Qt::white, QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER, 1);

    auto counterLabel = std::make_unique<Label>();
    counterLabel->setTextUnit(std::move(counterLabelUnit));
    counterLabel->setYOffset(Utils::scale_value(Margin::_8px));
    d->objects_[CounterLabel] = std::move(counterLabel);

    const auto authorFont(Fonts::appFontScaled(14, Fonts::FontWeight::Normal));
    auto authorLabelUnit = TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
    authorLabelUnit->init(authorFont, Qt::white, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);

    auto authorLabel = std::make_unique<Label>();
    authorLabel->setTextUnit(std::move(authorLabelUnit));
    authorLabel->setClickable(true);
    authorLabel->setDefaultColor(Qt::white);
    authorLabel->setHoveredColor(QColor(255, 255, 255, static_cast<int>(255 * 0.65)));
    authorLabel->setPressedColor(QColor(255, 255, 255, static_cast<int>(255 * 0.5)));
    d->objects_[AuthorLabel] = std::move(authorLabel);

    const auto dateFont(Fonts::appFontScaled(13, Fonts::FontWeight::Normal));
    auto dateLabelUnit = TextRendering::MakeTextUnit(QString());
    dateLabelUnit->init(dateFont, Qt::white, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);

    auto dateLabel = std::make_unique<Label>();
    dateLabel->setTextUnit(std::move(dateLabelUnit));
    dateLabel->setClickable(true);
    dateLabel->setDefaultColor(Qt::white);
    dateLabel->setHoveredColor(QColor(255, 255, 255, static_cast<int>(255 * 0.65)));
    dateLabel->setPressedColor(QColor(255, 255, 255, static_cast<int>(255 * 0.5)));
    d->objects_[DateLabel] = std::move(dateLabel);

    d->caption_ = std::make_unique<CaptionArea>(this);
    connect(d->caption_.get(), &CaptionArea::needHeight, this, &GalleryFrame::needHeight);

    connect(new QShortcut(QKeySequence::Copy, this), &QShortcut::activated, this, &GalleryFrame::copy);

    setMouseTracking(true);
}

GalleryFrame::~GalleryFrame()
{

}

void GalleryFrame::setFixedSize(const QSize& _size)
{
   QWidget::setFixedSize(_size);
   d->updateObjectsPositions(_size.width(), _size.height());

   update();
}

void GalleryFrame::setNextEnabled(bool _enable)
{
    auto & next = d->objects_[NextButton];
    next->setDisabled(!_enable);
    update(next->rect());
}

void GalleryFrame::setPrevEnabled(bool _enable)
{
    auto & prev = d->objects_[PrevButton];
    prev->setDisabled(!_enable);
    update(prev->rect());
}

void GalleryFrame::setZoomInEnabled(bool _enable)
{
    auto & zoomIn = d->objects_[ZoomInButton];
    zoomIn->setDisabled(!_enable);
    update(zoomIn->rect());
}

void GalleryFrame::setZoomOutEnabled(bool _enable)
{
    auto & zoomOut = d->objects_[ZoomOutButton];
    zoomOut->setDisabled(!_enable);
    update(zoomOut->rect());
}

void GalleryFrame::setSaveEnabled(bool _enable)
{
    auto & save = d->objects_[SaveButton];
    save->setDisabled(!_enable);
    update(save->rect());
}

void GalleryFrame::setMenuEnabled(bool _enable)
{
    auto & menu = d->objects_[MenuButton];
    menu->setDisabled(!_enable);
    update(menu->rect());
}

QString GalleryFrame::actionIconPath(Action action)
{
    switch (action)
    {
    case GoTo:
        return qsl(":/context_menu/goto");
    case Copy:
        return qsl(":/context_menu/copy");
    case Forward:
        return qsl(":/context_menu/forward");
    case SaveAs:
        return qsl(":/context_menu/download");
    case SaveToFavorites:
        return qsl(":/context_menu/favorites");
    default:
        Q_UNREACHABLE();
        return QString();
    }
}

QString GalleryFrame::actionText(Action action)
{
    switch (action)
    {
    case GoTo:
        return QT_TRANSLATE_NOOP("previewer", "Go to message");
    case Copy:
        return QT_TRANSLATE_NOOP("previewer", "Copy to clipboard");
    case Forward:
        return QT_TRANSLATE_NOOP("previewer", "Forward");
    case SaveAs:
        return QT_TRANSLATE_NOOP("previewer", "Save as");
    case SaveToFavorites:
        return QT_TRANSLATE_NOOP("previewer", "Add to favorites");
    default:
        Q_UNREACHABLE();
        return QString();
    }
}

void GalleryFrame::setMenuActions(Actions _actions)
{
    d->menuActions_ = _actions;
}

GalleryFrame::Actions GalleryFrame::menuActions() const
{
    return d->menuActions_;
}

void GalleryFrame::setZoomVisible(bool _visible)
{
    auto & zoomIn = d->objects_[ZoomInButton];
    auto & zoomOut = d->objects_[ZoomOutButton];

    zoomIn->setVisible(_visible);
    zoomOut->setVisible(_visible);

    update(zoomIn->rect());
    update(zoomOut->rect());
}

void GalleryFrame::setNavigationButtonsVisible(bool _visible)
{
    auto & next = d->objects_[NextButton];
    auto & prev = d->objects_[PrevButton];

    next->setVisible(_visible);
    prev->setVisible(_visible);

    update(next->rect());
    update(prev->rect());
}

void GalleryFrame::setCounterText(const QString &_text)
{
    setLabelText(CounterLabel, _text);
}

void GalleryFrame::setAuthorText(const QString &_text)
{
    setLabelText(AuthorLabel, _text);
}

void GalleryFrame::setDateText(const QString &_text)
{
    setLabelText(DateLabel, _text);
}

void GalleryFrame::setCaption(const QString& _caption)
{
    d->setCaption(_caption, width());
    update();
}

void GalleryFrame::setAuthorAndDateVisible(bool _visible)
{
    auto & author = d->objects_[AuthorLabel];
    auto & date = d->objects_[DateLabel];

    author->setVisible(_visible);
    date->setVisible(_visible);

    update(author->rect());
    update(date->rect());
}

void GalleryFrame::collapseCaption()
{
    d->collapseCaption();
    update();
}

bool GalleryFrame::isCaptionExpanded() const
{
    return d->isCaptionExpanded();
}

void GalleryFrame::onNext()
{
    closeMenu();
}

void GalleryFrame::onPrev()
{
    closeMenu();
}

void GalleryFrame::paintEvent(QPaintEvent* _event)
{
    QWidget::paintEvent(_event);
    const auto rect = _event->rect();

    QPainter p(this);

    p.setOpacity(backgroundOpacity);
    p.fillRect(0, 0, width(), height(), backgroundColor);

    for (auto & [_, object] : d->objects_)
    {
        if (object->rect().intersects(rect))
            object->draw(p);
    }
}

void GalleryFrame::mouseMoveEvent(QMouseEvent *_event)
{
    bool updateHover = _event->buttons() == Qt::NoButton;

    bool handCursor = false;
    for (auto & [_, object] : d->objects_)
    {
        const auto overObject = object->rect().contains(_event->pos());
        if (overObject || object->hovered())
        {
            handCursor |= overObject && object->clickable() && (updateHover || object->pressed());

            if (updateHover)
            {
                object->setHovered(overObject);
                update(object->rect());
            }
        }
    }

    setCursor(handCursor ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void GalleryFrame::mousePressEvent(QMouseEvent *_event)
{
    if (_event->button() != Qt::LeftButton)
        return QWidget::mousePressEvent(_event);

    for (auto & [_, object] : d->objects_)
    {
        if (object->rect().contains(_event->pos()) && object->clickable())
        {
            object->setPressed(true);
            update(object->rect());
        }
    }
}

void GalleryFrame::mouseReleaseEvent(QMouseEvent *_event)
{
    if (_event->button() != Qt::LeftButton)
        return QWidget::mouseReleaseEvent(_event);

    for (auto & [type, object] : d->objects_)
    {
        if (!object->clickable())
            continue;

        const auto overObject = object->rect().contains(_event->pos());
        object->setHovered(overObject);

        if (!object->disabled() && object->pressed() && overObject)
            onClick(type);

        object->setPressed(false);

        update(object->rect());
    }
}

bool GalleryFrame::event(QEvent *_event)
{
    return QWidget::event(_event);
}

void GalleryFrame::leaveEvent(QEvent *_event)
{
    d->resetObjects();
    repaint();

    QWidget::leaveEvent(_event);
}

void GalleryFrame::hideEvent(QHideEvent* _event)
{
    if (d->menu_)
        d->menu_->hide();

    return QWidget::hideEvent(_event);
}

void GalleryFrame::onClick(ControlType _type)
{
    switch (_type)
    {
        case AuthorLabel:
            Q_EMIT openContact();
            break;
        case DateLabel:
            Q_EMIT goToMessage();
            break;
        case ZoomOutButton:
            Q_EMIT zoomOut();
            break;
        case ZoomInButton:
            Q_EMIT zoomIn();
            break;
        case PrevButton:
            Q_EMIT prev();
            break;
        case NextButton:
            Q_EMIT next();
            break;
        case CloseButton:
            Q_EMIT close();
            break;
        case SaveButton:
            Q_EMIT save();
            break;
        case MenuButton:
        {
            auto buttonRect = d->objects_[MenuButton]->rect();
            auto pos = QPoint(buttonRect.x() + buttonRect.width() / 2, buttonRect.y());

            showMenuAt(mapToGlobal(pos));

            d->resetObjects();
        }
        break;

        default:
            break;
    }
}

void GalleryFrame::setLabelText(ControlType _type, const QString &_text)
{
    auto object = d->objects_[_type].get();
    if (object)
    {
        auto label = dynamic_cast<Label*>(object);
        label->setText(_text);
        update(label->rect());
    }
}

CustomMenu* GalleryFrame::createMenu()
{
    auto menu = new CustomMenu();

    if (d->menuActions_ & GoTo)
        addAction(GoTo, menu, this, &GalleryFrame::goToMessage);

    if (d->menuActions_ & Copy)
        addAction(Copy, menu, this, &GalleryFrame::copy);

    if (d->menuActions_ & Forward)
        addAction(Forward, menu, this, &GalleryFrame::forward);

    if (d->menuActions_ & SaveAs)
        addAction(SaveAs, menu, this, &GalleryFrame::saveAs);

    if (d->menuActions_ & SaveToFavorites)
        addAction(SaveToFavorites, menu, this, &GalleryFrame::saveToFavorites);

    return menu;
}
template<typename ...Args>
QAction* GalleryFrame::makeAction(Action action, CustomMenu* parent, Args&& ...args)
{
    auto a = new QAction(actionText(action), parent);
    connect(a, &QAction::triggered, std::forward<Args>(args)...);
    return a;
}

template<typename ...Args>
void GalleryFrame::addAction(Action action, CustomMenu* parent, Args && ...args)
{
    const auto iconSize = Utils::scale_value(QSize(20, 20));
    const auto iconColor = Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_SECONDARY);
    auto a = makeAction(action, parent, std::forward<Args>(args)...);
    parent->addAction(a, Utils::renderSvg(actionIconPath(action), iconSize, iconColor));
}

void GalleryFrame::showMenuAt(const QPoint& _pos)
{
    if (d->menu_)
        d->menu_->deleteLater();

    d->menu_ = createMenu();
    d->menu_->showAtPos(_pos);
}

void GalleryFrame::closeMenu()
{
    if (d->menu_)
        d->menu_->close();
}

class MenuItem : public BDrawable
{
public:
    virtual ~MenuItem() = default;
    void draw(QPainter &_p) override;

    Label label_;
    Icon icon_;
};

void MenuItem::draw(QPainter &_p)
{
    BDrawable::draw(_p);

    icon_.draw(_p);
    label_.draw(_p);
}

class CustomMenu_p
{
public:
    QFont font_;
    int itemHeight_;
    QSize arrowSize_;
    bool needUpdate_;
    QColor fontColor_;
    QColor hoverColor_;
    QColor pressedColor_;
    QColor backgroundColor_;

    using ItemData = std::pair<QAction*, std::unique_ptr<MenuItem>>;

    std::vector<ItemData> items_;


    int updateLabels(int _width)
    {
        const auto xOffsetLeft = Utils::scale_value(12);
        const auto xOffsetRight = Utils::scale_value(12);

        auto fm = QFontMetrics(font_);
        auto maxTextWidth = 0;
        auto maxTotalWidth = _width;
        for (auto & [action, menuItem] : items_)
            maxTextWidth = std::max(maxTextWidth, fm.horizontalAdvance(action->text()));

        const auto height = fm.height();

        for (auto i = 0u; i < items_.size(); ++i)
        {
            auto & label = items_[i].second->label_;
            auto & icon = items_[i].second->icon_;
            label.setXOffset(xOffsetLeft);
            label.setYOffset((itemHeight_ - height) / 2);

            icon.setXOffset(xOffsetLeft);
            icon.setYOffset((itemHeight_ - icon.pixmapSize().height()) / 2); // center icon by height

            icon.setRect(icon.isNullPixmap() ? QRect() : QRect(0, i * itemHeight_, xOffsetLeft + icon.pixmapSize().width(), itemHeight_));
            label.setRect(QRect(icon.rect().width(), i * itemHeight_, maxTextWidth + xOffsetLeft + xOffsetRight, itemHeight_));

            const auto itemWidth = icon.rect().width() + label.rect().width();
            items_[i].second->setRect(QRect(0, i * itemHeight_, itemWidth, itemHeight_));
            maxTotalWidth = std::max(maxTotalWidth, itemWidth);
        }

        needUpdate_ = false;

        return maxTotalWidth;
    }

    auto rectForUpdate(int itemIdx)
    {
        if (itemIdx >= static_cast<int>(items_.size()))
        {
            im_assert(!"wrong item index");
            return QRect();
        }

        auto updateRect = items_[itemIdx].second->rect();

        if (itemIdx == static_cast<int>(items_.size()) - 1) // last item updates with arrow
            updateRect.setBottom(updateRect.bottom() + arrowSize_.height());

        return updateRect;
    }

    void resetLabelsState()
    {
        for (auto & [_, label] : items_)
        {
            label->setHovered(false);
            label->setPressed(false);
        }
    }
};

CustomMenu::CustomMenu()
    : d(std::make_unique<CustomMenu_p>())
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    // set defaults
    d->arrowSize_ = Utils::scale_value(QSize(20, 8));
    d->itemHeight_ = Utils::scale_value(36);
    d->backgroundColor_ = Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE_PERMANENT);
    d->hoverColor_ = Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE_PERMANENT_HOVER);
    d->fontColor_ = Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    d->font_ = Fonts::appFontScaled(14, Fonts::FontWeight::Normal);
    d->needUpdate_ = false;

    setMouseTracking(true);
}

CustomMenu::~CustomMenu() = default;

void CustomMenu::addAction(QAction* _action, const QPixmap& _icon)
{
    auto item = std::make_unique<MenuItem>();
    item->background_ = Qt::transparent;
    item->hoveredBackground_ = d->hoverColor_;
    item->pressedBackground_ = d->pressedColor_;

    auto textUnit = TextRendering::MakeTextUnit(_action->text());
    textUnit->init(d->font_, d->fontColor_, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
    item->label_.setTextUnit(std::move(textUnit));

    item->icon_.setPixmap(_icon);

    d->items_.push_back(std::make_pair(_action, std::move(item)));

    d->needUpdate_ = true;
    updateHeight();
}

void CustomMenu::showAtPos(const QPoint &_pos)
{
    if (d->needUpdate_)
        setFixedWidth(d->updateLabels(minimumWidth()));

    const auto arrowPeak = QPoint(width() / 2,  height() + d->arrowSize_.height());
    move(_pos.x() - arrowPeak.x(), _pos.y() - arrowPeak.y());

    d->resetLabelsState();

#ifdef __APPLE__
    MacSupport::showOverAll(this);
#else
    show();
#endif
}

void CustomMenu::paintEvent(QPaintEvent *_event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath rectPath;
    rectPath.setFillRule(Qt::WindingFill);

    auto r = rect();
    r.setBottom(r.bottom() - d->arrowSize_.height());

    rectPath.addRoundedRect(r, getMenuBorderRadius(), getMenuBorderRadius());
    p.fillPath(rectPath, QBrush(d->backgroundColor_));                                      //    *********************
                                                                                            //    *                   *
    QPainterPath arrowPath;                                                                 //    *                   *
    arrowPath.setFillRule(Qt::WindingFill);                                                 //    *                   *
                                                                                            //    *                   *
    const auto a = QPoint((r.width() - d->arrowSize_.width()) / 2, r.height());             //    *******A     C*******
    const auto b = QPoint(r.width() / 2,  r.height() + d->arrowSize_.height());             //            *   *
    const auto c = QPoint((r.width() + d->arrowSize_.width()) / 2, r.height());             //              B

    arrowPath.moveTo(a);
    arrowPath.lineTo(b);
    arrowPath.lineTo(c);
    arrowPath.closeSubpath();

    // if last item is highlighted, then highlight arrow too
    QColor arrowColor = d->backgroundColor_;
    if (!d->items_.empty())
    {
        auto & label = d->items_.back().second;
        if (label->pressed() && label->pressedBackground_.isValid())
            arrowColor = label->pressedBackground_;
        else if (label->hovered() && label->hoveredBackground_.isValid())
            arrowColor = label->hoveredBackground_;
    }

    p.fillPath(arrowPath, arrowColor);

    p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    for (auto & [action, label] : d->items_)
    {
        if (label->rect().intersects(_event->rect()))
            label->draw(p);
    }
}

void CustomMenu::mouseMoveEvent(QMouseEvent *_event)
{
    if (_event->buttons() != Qt::NoButton)
        return QWidget::mouseMoveEvent(_event);

    for (auto i = 0u; i < d->items_.size(); i++)
    {
        auto & label = d->items_[i].second;
        const auto overObject = label->rect().contains(_event->pos());
        if (overObject || label->hovered())
        {
            label->setHovered(overObject);
            update(d->rectForUpdate(i));
        }
    }
}

void CustomMenu::mousePressEvent(QMouseEvent *_event)
{
    if (_event->button() != Qt::LeftButton)
        return QWidget::mousePressEvent(_event);

    for (auto i = 0u; i < d->items_.size(); i++)
    {
        auto & label = d->items_[i].second;
        if (label->rect().contains(_event->pos()))
        {
            label->setPressed(true);
            update(d->rectForUpdate(i));
        }
    }

    QWidget::mousePressEvent(_event);
}

void CustomMenu::mouseReleaseEvent(QMouseEvent *_event)
{
    if (_event->button() != Qt::LeftButton)
        return QWidget::mouseReleaseEvent(_event);

    for (auto i = 0u; i < d->items_.size(); i++)
    {
        auto & [action, label] = d->items_[i];
        const auto overObject = label->rect().contains(_event->pos());
        label->setHovered(overObject);

        if (label->pressed() && overObject)
        {
            close();
            action->trigger();
        }

        label->setPressed(false);

        update(d->rectForUpdate(i));
    }
}

void CustomMenu::updateHeight()
{
    setFixedHeight(d->items_.size() * d->itemHeight_ + d->arrowSize_.height());
}

AccessibleGalleryFrame::AccessibleGalleryFrame(GalleryFrame* widget)
    : QAccessibleWidget(widget)
{
    static constexpr const std::array<ControlType, 7> controlTypes{
        ControlType::ZoomOutButton,
        ControlType::ZoomInButton,
        ControlType::PrevButton,
        ControlType::NextButton,
        ControlType::CloseButton,
        ControlType::MenuButton,
        ControlType::SaveButton,
    };

    auto galleryFrame = qobject_cast<GalleryFrame*>(object());
    for (auto control : controlTypes)
    {
        auto drawable = galleryFrame->d->objects_.at(control).get();
        auto controlObject = dynamic_cast<QObject*>(drawable);
        im_assert(controlObject);
        if (controlObject)
        {
            auto accessibleControlObject = QAccessible::queryAccessibleInterface(controlObject);
            im_assert(accessibleControlObject);
            children_.emplace_back(accessibleControlObject);
        }
    }
}

int AccessibleGalleryFrame::childCount() const
{
    return children_.size();
}

QAccessibleInterface* AccessibleGalleryFrame::child(int index) const
{
    if (index > -1 && index < static_cast<int>(children_.size()))
        return children_.at(index);
    return nullptr;
}

int AccessibleGalleryFrame::indexOfChild(const QAccessibleInterface* child) const
{
    return Utils::indexOf(children_.cbegin(), children_.cend(), child);
}

}
