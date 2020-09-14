#include "stdafx.h"

#include "HistoryControlPageItem.h"
#include "../../gui_settings.h"
#include "../../utils/utils.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/InterConnector.h"
#include "../../controls/TooltipWidget.h"
#include "../../styles/ThemeParameters.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/FavoritesUtils.h"
#include "../containers/FriendlyContainer.h"
#include "../MainPage.h"
#include "../ContactDialog.h"
#include "MessageStyle.h"
#include "LastStatusAnimation.h"

namespace
{
    constexpr int ANIM_MAX_VALUE = 100;
    constexpr int ADD_OFFSET = 10;
    constexpr int MULTISELECT_MARK_SIZE = 20;
    constexpr int MULTISELECT_MARK_OFFSET = 12;
}

namespace Ui
{
    HistoryControlPageItem::HistoryControlPageItem(QWidget *parent)
        : QWidget(parent)
        , QuoteAnimation_(parent)
        , isChat_(false)
        , Selected_(false)
        , HasTopMargin_(false)
        , HasAvatar_(false)
        , hasSenderName_(false)
        , isChainedToPrev_(false)
        , isChainedToNext_(false)
        , HasAvatarSet_(false)
        , isDeleted_(false)
        , isEdited_(false)
        , isContextMenuEnabled_(true)
        , isMultiselectEnabled_(true)
        , selectedTop_(false)
        , selectedBottom_(false)
        , hoveredTop_(false)
        , hoveredBottom_(false)
        , selectionCenter_(0)
        , lastStatus_(LastStatus::None)
        , lastStatusAnimation_(nullptr)
        , addAnimation_(nullptr)
        , removeAnimation_(nullptr)
        , heightAnimation_(nullptr)
        , intersected_(false)
        , wasSelected_(false)
        , isUnsupported_(false)
        , nextHasSenderName_(false)
        , nextIsOutgoing_(false)
        , initialized_(false)
    {
        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &HistoryControlPageItem::avatarChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectSpaceClicked, this, [this]
        {
            if (Utils::InterConnector::instance().isMultiselect() && Utils::InterConnector::instance().currentMultiselectMessage() == getId())
            {
                setSelected(!isSelected());
                if (isSelected())
                    Q_EMIT Utils::InterConnector::instance().messageSelected(getId(), getInternalId());
                update();
            }
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::selectionStateChanged, this, [this] (qint64 _id, const QString& _internalId, bool _selected)
        {
            if (prev_.getId() == _id && prev_.getInternalId() == _internalId)
            {
                selectedTop_ = _selected;
                update();
            }
            else if (next_.getId() == _id && next_.getInternalId() == _internalId)
            {
                selectedBottom_ = _selected;
                update();
            }
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiSelectCurrentMessageChanged, this, [this]
        {
            auto id = Utils::InterConnector::instance().currentMultiselectMessage();
            if (prev_.getId() == id && id != -1)
            {
                hoveredTop_ = true;
            }
            else if (next_.getId() == id && id != -1)
            {
                hoveredBottom_ = true;
            }
            else
            {
                hoveredBottom_ = false;
                hoveredTop_ = false;
            }
            update();
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, [this]
        {
            if (!Utils::InterConnector::instance().isMultiselect(aimId_))
                selectionCenter_ = 0;
        });

        connect(get_gui_settings(), &qt_gui_settings::changed, this, [this](const QString& _key)
        {
            if (_key == ql1s(settings_show_reactions))
                onReactionsEnabledChanged();
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated,
                this, &HistoryControlPageItem::onReactionsEnabledChanged);

        setMouseTracking(true);
    }

    bool HistoryControlPageItem::isOutgoingPosition() const
    {
        return msg_.isOutgoingPosition();
    }

    void HistoryControlPageItem::clearSelection(bool)
    {
        if (Selected_)
            update();

        Selected_ = false;
        Q_EMIT selectionChanged();
        Q_EMIT Utils::InterConnector::instance().updateSelectedCount();
    }

    bool HistoryControlPageItem::hasAvatar() const
    {
        return HasAvatar_;
    }

    bool HistoryControlPageItem::hasSenderName() const
    {
        return hasSenderName_;
    }

    bool HistoryControlPageItem::hasTopMargin() const
    {
        return HasTopMargin_;
    }

    void HistoryControlPageItem::setSelected(const bool _isSelected)
    {
        const auto prevState = Selected_;
        if (prevState != _isSelected)
        {
            update();
            Q_EMIT selectionChanged();
        }

        Selected_ = _isSelected;

        if (prevState != _isSelected)
        {
            Q_EMIT Utils::InterConnector::instance().selectionStateChanged(getId(), getInternalId(), Selected_);
            Q_EMIT Utils::InterConnector::instance().updateSelectedCount();
        }
    }

    void HistoryControlPageItem::setTopMargin(const bool value)
    {
        if (HasTopMargin_ == value)
            return;

        HasTopMargin_ = value;

        updateSize();
        updateGeometry();
        update();
    }

    void HistoryControlPageItem::selectByPos(const QPoint&, const QPoint&, const QPoint&, const QPoint&)
    {

    }

    bool HistoryControlPageItem::isSelected() const
    {
        return Selected_;
    }

    void HistoryControlPageItem::onActivityChanged(const bool isActive)
    {
        const auto isInit = (isActive && !initialized_);
        if (isInit)
        {
            initialized_ = true;
            initialize();
            if (isEditable())
                startSpellChecking();
        }
    }

    void HistoryControlPageItem::onVisibilityChanged(const bool /*isVisible*/)
    {
    }

    void HistoryControlPageItem::onDistanceToViewportChanged(const QRect& /*_widgetAbsGeometry*/, const QRect& /*_viewportVisibilityAbsRect*/)
    {}

    void HistoryControlPageItem::setHasAvatar(const bool value)
    {
        HasAvatar_ = value;
        HasAvatarSet_ = true;

        updateGeometry();
    }

    void HistoryControlPageItem::setHasSenderName(const bool _hasSender)
    {
        hasSenderName_ = _hasSender;
        updateGeometry();
    }

    void HistoryControlPageItem::setChainedToPrev(const bool _isChained)
    {
        if (isChainedToPrev_ != _isChained)
        {
            isChainedToPrev_ = _isChained;

            onChainsChanged();
            updateGeometry();
        }
    }

    void HistoryControlPageItem::setChainedToNext(const bool _isChained)
    {
        if (isChainedToNext_ != _isChained)
        {
            isChainedToNext_ = _isChained;

            onChainsChanged();
            updateSize();
            updateGeometry();
            update();
        }
    }

    void HistoryControlPageItem::setPrev(const Logic::MessageKey& _key)
    {
        prev_ = _key;
    }

    void HistoryControlPageItem::setNext(const Logic::MessageKey& _key)
    {
        next_= _key;
    }

    void HistoryControlPageItem::setContact(const QString& _aimId)
    {
        assert(!_aimId.isEmpty());

        if (_aimId == aimId_)
            return;

        assert(aimId_.isEmpty());

        aimId_ = _aimId;
        QuoteAnimation_.setAimId(_aimId);
    }

    void HistoryControlPageItem::setSender(const QString& /*_sender*/)
    {

    }

    void HistoryControlPageItem::updateFriendly(const QString&, const QString&)
    {
    }

    void HistoryControlPageItem::setDeliveredToServer(const bool _delivered)
    {

    }

    void HistoryControlPageItem::drawLastStatusIconImpl(QPainter& _p, int _rightPadding, int _bottomPadding)
    {
        if (lastStatusAnimation_)
        {
            const QRect rc = rect();

            const auto iconSize = MessageStyle::getLastStatusIconSize();

            lastStatusAnimation_->drawLastStatus(
                _p,
                rc.right() - MessageStyle::getRightBubbleMargin() - _rightPadding + MessageStyle::getLastStatusLeftMargin(),
                rc.bottom() - iconSize.height() - _bottomPadding - bottomOffset(),
                iconSize.width(),
                iconSize.height());
        }
    }

    int HistoryControlPageItem::maxHeadsCount() const
    {
        if (headsAtBottom())
            return 5;

        const auto w = width();
        if (w >= MessageStyle::fiveHeadsWidth())
            return 5;

        if (w >= MessageStyle::fourHeadsWidth())
            return 4;

        return 3;
    }

    void HistoryControlPageItem::ensureAddAnimationInitialized()
    {
        if (addAnimation_)
            return;

        addAnimation_ = new QVariantAnimation(this);
        addAnimation_->setStartValue(0);
        addAnimation_->setEndValue(ANIM_MAX_VALUE);
        addAnimation_->setDuration(200);
        addAnimation_->setEasingCurve(QEasingCurve::OutExpo);
        connect(addAnimation_, &QVariantAnimation::valueChanged, this, qOverload<>(&HistoryControlPageItem::update));
        connect(addAnimation_, &QVariantAnimation::finished, this, [this]()
        {
            for (auto& h : heads_)
                h.adding_ = false;

            updateGeometry();
            updateSize();
        });
    }

    void HistoryControlPageItem::ensureRemoveAnimationInitialized()
    {
        if (removeAnimation_)
            return;

        removeAnimation_ = new QVariantAnimation(this);
        removeAnimation_->setStartValue(ANIM_MAX_VALUE);
        removeAnimation_->setEndValue(0);
        removeAnimation_->setEasingCurve(QEasingCurve::OutExpo);
        connect(removeAnimation_, &QVariantAnimation::valueChanged, this, qOverload<>(&HistoryControlPageItem::update));
        connect(removeAnimation_, &QVariantAnimation::finished, this, [this]()
        {
            heads_.erase(std::remove_if(heads_.begin(), heads_.end(),
                [](const auto& x) { return x.removing_; }),
                heads_.end());

            updateGeometry();
            updateSize();
        });
    }

    void HistoryControlPageItem::ensureHeightAnimationInitialized()
    {
        if (heightAnimation_)
            return;
        
        heightAnimation_ = new QVariantAnimation(this);
        heightAnimation_->setStartValue(0);
        heightAnimation_->setEndValue(ANIM_MAX_VALUE);
        heightAnimation_->setDuration(200);
        heightAnimation_->setEasingCurve(QEasingCurve::OutExpo);
        connect(heightAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant&)
        {
            updateGeometry();
            update();
            updateSize();
        });
    }

    void HistoryControlPageItem::avatarChanged(const QString& _aimid)
    {
        if (std::any_of(heads_.cbegin(), heads_.cend(), [&_aimid](const auto& h) { return h.aimid_ == _aimid; }))
            update();
    }

    void HistoryControlPageItem::onReactionsEnabledChanged()
    {
        if (!supportsReactions())
            return;

        if (MessageReactions::enabled() && !reactions_)
        {
            reactions_ = std::make_unique<MessageReactions>(this);

            if (msg_.reactions_ && msg_.reactions_->exist_)
                reactions_->subscribe();
        }
        else if (!MessageReactions::enabled())
        {
            if (reactions_)
                reactions_->deleteControls();

            reactions_.reset();
            updateSize();
        }
    }

    void HistoryControlPageItem::drawLastStatusIcon(QPainter& _p,  LastStatus _lastStatus, const QString& _aimid, const QString& _friendly, int _rightPadding)
    {
        if (!heads_.isEmpty() || Utils::InterConnector::instance().isMultiselect())
            return;

        switch (lastStatus_)
        {
        case LastStatus::None:
        case LastStatus::Read:
            break;
        case LastStatus::Pending:
        case LastStatus::DeliveredToServer:
        case LastStatus::DeliveredToPeer:
            drawLastStatusIconImpl(_p, _rightPadding, 0);
            break;
        default:
            break;
        }
    }

    void HistoryControlPageItem::mouseMoveEvent(QMouseEvent* e)
    {
        if (Utils::InterConnector::instance().isMultiselect())
        {
            if (isMultiselectEnabled())
                setCursor(Qt::PointingHandCursor);
        }
        else if (!heads_.isEmpty())
        {
            if (showHeadsTooltip(rect(), e->pos()))
                setCursor(Qt::PointingHandCursor);
        }

        if (reactions_)
            reactions_->onMouseMove(e->pos());

        return QWidget::mouseMoveEvent(e);
    }

    void HistoryControlPageItem::mousePressEvent(QMouseEvent* e)
    {
        pressPoint = e->pos();

        if (reactions_)
            reactions_->onMousePress(e->pos());

        return QWidget::mousePressEvent(e);
    }

    void HistoryControlPageItem::mouseReleaseEvent(QMouseEvent* e)
    {
        const auto pos = e->pos();
        if (Utils::clicked(pressPoint, pos))
        {
            if (Utils::InterConnector::instance().isMultiselect() && isMultiselectEnabled())
            {
                if (getId() != -1 && e->button() == Qt::LeftButton)
                {
                    setSelected(!isSelected());
                    if (isSelected())
                        Q_EMIT Utils::InterConnector::instance().messageSelected(getId(), getInternalId());

                    update();
                }
            }
            else if (!heads_.isEmpty())
            {
                clickHeads(rect(), pos, e->button() == Qt::LeftButton);
            }
        }

        if (!Utils::InterConnector::instance().isMultiselect() && reactions_)
            reactions_->onMouseRelease(pos);

        return QWidget::mouseReleaseEvent(e);
    }

    void HistoryControlPageItem::enterEvent(QEvent* _event)
    {
        Utils::InterConnector::instance().setCurrentMultiselectMessage(isMultiselectEnabled() ? getId() : -1);
        QWidget::enterEvent(_event);
    }

    void HistoryControlPageItem::leaveEvent(QEvent* _event)
    {
        if (reactions_)
            reactions_->onMouseLeave();

        QWidget::leaveEvent(_event);
    }

    bool HistoryControlPageItem::showHeadsTooltip(const QRect& _rc, const QPoint& _pos)
    {
        const int avatarSize = MessageStyle::getLastReadAvatarSize();
        auto xMargin = avatarSize + MessageStyle::getLastReadAvatarMargin();
        auto maxCount = maxHeadsCount();
        auto i = heads_.size() > maxCount ? maxCount : heads_.size() - 1;
        for (; i >= 0; --i)
        {
            const auto r = QRect(_rc.right() - xMargin, _rc.bottom() + 1 - avatarSize - MessageStyle::getLastReadAvatarBottomMargin(), avatarSize, avatarSize);
            if (r.contains(_pos))
            {
                auto text = heads_[i].friendly_;
                if (i == maxCount)
                {
                    for (auto j = i + 1; j < heads_.size(); ++j)
                    {
                        text += QChar::LineFeed;
                        text += heads_[j].friendly_;
                    }
                }

                if (!text.isEmpty())
                    Tooltip::show(text, QRect(mapToGlobal(r.topLeft()), QSize(avatarSize, avatarSize)));

                return i != maxCount || (i == maxCount && heads_.size() == maxCount + 1);
            }

            xMargin += (avatarSize + MessageStyle::getLastReadAvatarOffset());
        }

        return false;
    }

    bool HistoryControlPageItem::clickHeads(const QRect& _rc, const QPoint& _pos, bool _left)
    {
        const int avatarSize = MessageStyle::getLastReadAvatarSize();
        auto xMargin = avatarSize + MessageStyle::getLastReadAvatarMargin();
        auto maxCount = maxHeadsCount();
        auto i = heads_.size() > maxCount ? maxCount : heads_.size() - 1;
        for (; i >= 0; --i)
        {
            const auto r = QRect(_rc.right() - xMargin, _rc.bottom() + 1 - avatarSize - MessageStyle::getLastReadAvatarBottomMargin(), avatarSize, avatarSize);
            if (r.contains(_pos) && (i != maxCount || (i == maxCount && heads_.size() == maxCount + 1)))
            {
                if (_left)
                {
                    Utils::openDialogOrProfile(heads_.at(i).aimid_, Utils::OpenDOPParam::aimid);
                    Tooltip::hide();
                }
                else
                {
                    Q_EMIT mention(heads_.at(i).aimid_, heads_.at(i).friendly_);
                }
                return true;
            }

            xMargin += (avatarSize + MessageStyle::getLastReadAvatarOffset());
        }

        return false;
    }

    bool HistoryControlPageItem::handleSelectByPos(const QPoint& from, const QPoint& to, const QPoint& areaFrom, const QPoint& areaTo)
    {
        if (Utils::InterConnector::instance().isMultiselect())
        {
            if (prevFrom_ != areaFrom && prevTo_ != areaTo)
            {
                intersected_ = false;
                wasSelected_ = isSelected();
            }

            const auto isSelectionTopToBottom = (from.y() <= to.y());
            const auto &topPoint = (isSelectionTopToBottom ? from : to);
            const auto &bottomPoint = (isSelectionTopToBottom ? to : from);

            const auto y = selectionCenter_ == 0 ? mapToGlobal(rect().center()).y() : mapToGlobal(QPoint(0, selectionCenter_)).y();

            const auto intersects = QRect(topPoint, bottomPoint).intersects(QRect(QPoint(mapToGlobal(rect().topLeft()).x(), y), QSize(rect().width(), 1)));
            if (intersected_ == intersects)
                return true;

            if (intersects)
                setSelected(!isSelected());
            else if (intersected_)
                setSelected(wasSelected_);

            intersected_ = intersects;
            prevFrom_ = areaFrom;
            prevTo_ = areaTo;

            update();
            return true;
        }

        return false;
    }

    void HistoryControlPageItem::initialize()
    {
        if (supportsReactions() && MessageReactions::enabled())
        {
            reactions_ = std::make_unique<MessageReactions>(this);
            const auto& reactions = buddy().reactions_;
            if (reactions && reactions->exist_)
                reactions_->subscribe();
        }
    }

    void HistoryControlPageItem::onSizeChanged()
    {
        if (reactions_)
            reactions_->onMessageSizeChanged();
    }

    ReactionsPlateType HistoryControlPageItem::reactionsPlateType() const
    {
        return ReactionsPlateType::Regular;
    }

    bool HistoryControlPageItem::hasReactions() const
    {
        return msg_.reactions_ && msg_.reactions_->exist_;
    }

    QRect HistoryControlPageItem::reactionsPlateRect() const
    {
        if (reactions_)
            return reactions_->plateRect();

        return QRect();
    }

    bool HistoryControlPageItem::hasHeads() const noexcept
    {
        return !heads_.isEmpty();
    }

    bool HistoryControlPageItem::headsAtBottom() const
    {
        if (isOutgoing())
            return true;

        auto dialog = Utils::InterConnector::instance().getContactDialog();
        if (!dialog)
            return false;

        return dialog->width() < Utils::scale_value(460);
    }

    QMap<QString, QVariant> HistoryControlPageItem::makeData(const QString& _command, const QString& _arg)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;

        if (!_arg.isEmpty())
            result[qsl("arg")] = _arg;

        return result;
    }

    bool HistoryControlPageItem::isChainedToPrevMessage() const
    {
        return isChainedToPrev_;
    }

    bool HistoryControlPageItem::isChainedToNextMessage() const
    {
        return isChainedToNext_;
    }

    bool HistoryControlPageItem::isContextMenuReplyOnly() const noexcept
    {
        return Utils::InterConnector::instance().isRecordingPtt();
    }

    void HistoryControlPageItem::setSelectionCenter(int _center)
    {
        selectionCenter_ = _center;
    }

    void HistoryControlPageItem::setIsUnsupported(bool _unsupported)
    {
        isUnsupported_ = _unsupported;
        setContextMenuEnabled(!isUnsupported_);
    }

    bool HistoryControlPageItem::isUnsupported() const
    {
        return isUnsupported_;
    }

    bool HistoryControlPageItem::isEditable() const
    {
        const auto channelAdmin = Logic::getContactListModel()->isChannel(aimId_) && Logic::getContactListModel()->isYouAdmin(aimId_);
        const auto readOnlyChannel = Logic::getContactListModel()->isChannel(aimId_) && Logic::getContactListModel()->isReadonly(aimId_);
        return !readOnlyChannel && (isOutgoing() || channelAdmin);
    }

    void HistoryControlPageItem::setNextHasSenderName(bool _nextHasSenderName)
    {
        nextHasSenderName_ = _nextHasSenderName;
        updateSize();
    }

    void HistoryControlPageItem::setBuddy(const Data::MessageBuddy& _msg)
    {
        const auto reactionsAdedd = (!msg_.reactions_ || !msg_.reactions_->exist_) && _msg.reactions_ && _msg.reactions_->exist_;

        msg_ = _msg;

        if (reactions_ && initialized_ && reactionsAdedd)
            reactions_->subscribe();
    }

    const Data::MessageBuddy& HistoryControlPageItem::buddy() const
    {
        return msg_;
    }

    Data::MessageBuddy& HistoryControlPageItem::buddy()
    {
        return msg_;
    }

    bool HistoryControlPageItem::nextHasSenderName() const
    {
        return nextHasSenderName_;
    }

    void HistoryControlPageItem::setNextIsOutgoing(bool _nextIsOutgoing)
    {
        nextIsOutgoing_ = _nextIsOutgoing;
        updateSize();
    }

    bool HistoryControlPageItem::nextIsOutgoing() const
    {
        return nextIsOutgoing_;
    }

    QRect HistoryControlPageItem::messageRect() const
    {
        return QRect();
    }

    void HistoryControlPageItem::setReactions(const Data::Reactions& _reactions)
    {
        if (reactions_)
        {
            reactions_->setReactions(_reactions);
            update();
        }
    }

    void HistoryControlPageItem::drawSelection(QPainter& _p, const QRect& _rect)
    {
        static auto size = QSize(Utils::scale_value(MULTISELECT_MARK_SIZE), Utils::scale_value(MULTISELECT_MARK_SIZE));

        static auto on = Utils::renderSvgLayered(qsl(":/history/multiselect_on"),
            {
                {qsl("circle"), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY)},
                {qsl("check"), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT)}
            },
            size);

        static auto off = Utils::renderSvg(qsl(":/history/multiselect_off"), size, Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));

        selectionMarkRect_ = QRect(QPoint(width() - Utils::scale_value(MULTISELECT_MARK_OFFSET) - size.width(), _rect.center().y() - size.height() / 2), size);

        double current = Utils::InterConnector::instance().multiselectAnimationCurrent() / 100.0;

        {
            Utils::PainterSaver ps(_p);
            _p.setOpacity(current);
            _p.drawPixmap(selectionMarkRect_.x(), selectionMarkRect_.y(), off);
            if (isSelected())
            {
                auto min = Utils::scale_value(8);
                auto w = (int)(((size.width() - min) * current + min) / 2) * 2;
                auto h = (int)(((size.height() - min) * current + min) / 2) * 2;
                _p.drawPixmap(selectionMarkRect_.x() + (size.width() - w) / 2.0, selectionMarkRect_.y() + (size.width() - h) / 2.0, w, h, on);
            }
        }

        const auto hoverColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL, 0.05 * current);
        const auto hoverAndSelectedColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL, 0.15 * current);
        const auto selectionColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL, 0.10 * current);

        auto r = rect();
        if (HasTopMargin_ && !selectedTop_ && !hoveredTop_)
            r.setY(r.y() + (MessageStyle::getTopMargin(true) - MessageStyle::getTopMargin(false)));
        if (!selectedBottom_ && !hoveredBottom_ && bottomOffset() != Utils::getShadowMargin())
            r.setHeight(r.height() - (bottomOffset() - Utils::getShadowMargin()));

        const auto hovered = (Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Message && Utils::InterConnector::instance().currentMultiselectMessage() == getId());

        if (selectedTop_ || hoveredTop_)
        {
            auto topRect = rect();
            topRect.setHeight(topRect.height() - r.height());
            auto color = selectedTop_ ? (hoveredTop_ ? hoverAndSelectedColor : selectionColor) : hoverColor;
            if ((hovered || isSelected()) && !topRect.intersects(r))
                _p.fillRect(topRect, color);
        }

        if (selectedBottom_ || hoveredBottom_)
        {
            auto bottomRect = rect();
            bottomRect.setY(r.y() + r.height());
            auto color = selectedBottom_ ? (hoveredBottom_ ? hoverAndSelectedColor : selectionColor) : hoverColor;
            if ((hovered || isSelected()) && !bottomRect.intersects(r))
                _p.fillRect(bottomRect, color);
        }

        if (hovered)
        {
            if (isSelected())
            {
                _p.fillRect(r, hoverAndSelectedColor);
                return;
            }
            else
            {
                _p.fillRect(r, hoverColor);
            }
        }

        if (!isSelected())
            return;

        _p.fillRect(r, selectionColor);
    }

    void HistoryControlPageItem::startSpellChecking()
    {
    }

    void HistoryControlPageItem::drawHeads(QPainter& _p) const
    {
        if (heads_.isEmpty() || Utils::InterConnector::instance().isMultiselect() || Favorites::isFavorites(aimId_))
            return;

        const QRect rc = rect();

        const int avatarSize = MessageStyle::getLastReadAvatarSize();
        const int size = Utils::scale_bitmap(avatarSize);

        auto xMargin = avatarSize + MessageStyle::getLastReadAvatarMargin() - Utils::scale_value(1);
        auto addMargin = 0, addingCount = 0, delCount = 0;
        const auto isAddAnimationRunning = addAnimation_ && addAnimation_->state() == QAbstractAnimation::State::Running;
        const auto curAdd = isAddAnimationRunning ? addAnimation_->currentValue().toDouble() / ANIM_MAX_VALUE : 0.0;
        const auto isRemoveAnimationRunning = removeAnimation_ && removeAnimation_->state() == QAbstractAnimation::State::Running;
        const auto curDel = isRemoveAnimationRunning ? removeAnimation_->currentValue().toDouble() / ANIM_MAX_VALUE : static_cast<double>(ANIM_MAX_VALUE);
        for (const auto& h : heads_)
        {
            if (h.adding_)
            {
                addMargin += Utils::scale_value(ADD_OFFSET) - (Utils::scale_value(ADD_OFFSET) * curAdd);
                addingCount++;
            }
            if (h.removing_)
            {
                delCount++;
            }
        }

        auto maxCount = maxHeadsCount();
        const auto addPhAnimated = addMargin != 0 && heads_.size() - addingCount == maxCount;
        const auto delPhAnimated = curDel != ANIM_MAX_VALUE && heads_.size() - delCount <= maxCount;

        auto i = heads_.size() > maxCount ? maxCount : heads_.size() - 1;
        if (addPhAnimated && i < (heads_.size() - 1))
            i++;

        static const auto morePlaceholder = Utils::loadPixmap(qsl(":/history/i_more_seens_100"));

        for (; i >= 0; --i)
        {
            Utils::PainterSaver ps(_p);

            bool isDefault = false;
            QPixmap avatar;
            if (i == maxCount && !addPhAnimated && heads_.size() > maxCount + 1)
                avatar = morePlaceholder;
            else
                avatar = *Logic::GetAvatarStorage()->GetRounded(heads_[i].aimid_, heads_[i].friendly_, size, isDefault, false, false);

            auto margin = xMargin;
            const auto isAddAnimationRunning = addAnimation_ && addAnimation_->state() == QAbstractAnimation::State::Running;
            if (heads_[i].adding_ && isAddAnimationRunning && i != maxCount)
            {
                _p.setOpacity(curAdd);
                margin += addMargin;
            }
            else if (heads_[i].removing_ || (i == maxCount && delPhAnimated))
            {
                _p.setOpacity(curDel);
            }

            _p.drawPixmap(
                rc.right() - margin,
                rc.bottom() + 1 - avatarSize - MessageStyle::getLastReadAvatarBottomMargin(),
                avatarSize,
                avatarSize,
                avatar);

            if (i == (maxCount) && addPhAnimated)
            {
                avatar = morePlaceholder;
                _p.drawPixmap(
                    rc.right() - xMargin,
                    rc.bottom() + 1 - avatarSize - MessageStyle::getLastReadAvatarBottomMargin() - (avatarSize - avatarSize * curAdd),
                    avatarSize,
                    avatarSize,
                    avatar);
            }

            xMargin += (avatarSize + MessageStyle::getLastReadAvatarOffset());
        }
    }

    qint64 HistoryControlPageItem::getId() const
    {
        return -1;
    }

    qint64 HistoryControlPageItem::getPrevId() const
    {
        return -1;
    }

    const QString& HistoryControlPageItem::getInternalId() const
    {
        static const QString _id;
        return _id;
    }

    QString HistoryControlPageItem::getSourceText() const
    {
        return QString();
    }

    void HistoryControlPageItem::setDeleted(const bool _isDeleted)
    {
        isDeleted_ = _isDeleted;
    }

    bool HistoryControlPageItem::isDeleted() const
    {
        return isDeleted_;
    }

    void HistoryControlPageItem::setEdited(const bool _isEdited)
    {
        isEdited_ = _isEdited;
    }

    bool HistoryControlPageItem::isEdited() const
    {
        return isEdited_;
    }

    void HistoryControlPageItem::setLastStatus(LastStatus _lastStatus)
    {
        if (lastStatus_ == _lastStatus)
            return;

        lastStatus_ = _lastStatus;

        if (Favorites::isFavorites(aimId_) && lastStatus_ == LastStatus::Read)
            lastStatus_ = LastStatus::DeliveredToPeer;

        if (lastStatus_ != LastStatus::None)
        {
            if (!lastStatusAnimation_)
                lastStatusAnimation_ = new LastStatusAnimation(this);
        }

        if (lastStatusAnimation_)
        {
            lastStatusAnimation_->setLastStatus(lastStatus_);
            showMessageStatus();
        }

        if (!isChat_)
        {
            if (lastStatus_ == LastStatus::Read)
            {
                Data::ChatHead h;
                h.aimid_ = aimId_;
                h.friendly_ = Logic::GetFriendlyContainer()->getFriendly(aimId_);
                setHeads({ std::move(h) });
            }
            else
            {
                setHeads({});
            }
        }
    }

    LastStatus HistoryControlPageItem::getLastStatus() const
    {
        return lastStatus_;
    }

    int HistoryControlPageItem::bottomOffset() const
    {
        auto margin =  Utils::getShadowMargin();

        if (isChat() && hasHeads() && headsAtBottom())
        {
            margin += MessageStyle::getLastReadAvatarSize() + MessageStyle::getLastReadAvatarOffset();
        }
        else if (isChat() && isOutgoing() && !nextIsOutgoing())
        {
            auto single = false;
            auto dialog = Utils::InterConnector::instance().getContactDialog();
            if (dialog)
                single = dialog->width() < Utils::scale_value(460);

            margin += single ? MessageStyle::getTopMargin(true) : (MessageStyle::getLastReadAvatarSize() + MessageStyle::getLastReadAvatarOffset());
        }

        auto reactionsMargin = 0;

        const auto emptyReactions = reactions_ && !reactions_->hasReactions();

        if (hasReactions() && !emptyReactions && MessageReactions::enabled())
        {
            reactionsMargin += MessageStyle::Reactions::plateHeight();
            if (reactionsPlateType() == ReactionsPlateType::Media)
                reactionsMargin += MessageStyle::Reactions::mediaPlateYOffset();
            else
                reactionsMargin += MessageStyle::Reactions::plateYOffset() + MessageStyle::Reactions::shadowHeight();
        }

        return margin + reactionsMargin;
    }

    void HistoryControlPageItem::setHeads(const Data::HeadsVector& _heads)
    {
        if (!isChat_)
        {
            if (heads_.empty() && !_heads.empty())
                return addHeads(_heads);
            else if (!heads_.empty() && _heads.empty())
                return removeHeads(heads_);
        }

        if (areTheSameHeads(_heads))
            return;

        heads_.clear();
        for (const auto& h : _heads)
        {
            if (!heads_.contains(h))
                heads_.push_back(h);
        }

        updateSize();
        updateGeometry();
        update();
    }

    void HistoryControlPageItem::addHeads(const Data::HeadsVector& _heads)
    {
        if (areTheSameHeads(_heads))
            return;

        for (const auto& h : _heads)
            heads_.removeAll(h);

        const auto empty = heads_.isEmpty();

        for (const auto& h : _heads)
        {
            heads_.push_front(h);
            heads_.front().adding_ = !empty;
        }

        if (empty)
        {
            if (headsAtBottom())
            {
                ensureHeightAnimationInitialized();
                if (heightAnimation_->state() != QAbstractAnimation::State::Running)
                {
                    heightAnimation_->stop();
                    heightAnimation_->setDirection(QAbstractAnimation::Forward);
                    heightAnimation_->start();
                }
            }
        }
        else
        {
            ensureAddAnimationInitialized();
            if (addAnimation_->state() != QAbstractAnimation::State::Running)
                addAnimation_->start();
        }


        updateGeometry();
        update();
    }

    void HistoryControlPageItem::removeHeads(const Data::HeadsVector& _heads)
    {
        auto delCount = 0;
        for (const auto& h : _heads)
        {
            const auto ind = heads_.indexOf(h);
            if (ind == -1)
                continue;

            heads_[ind].removing_ = true;
            ++delCount;
        }

        ensureRemoveAnimationInitialized();
        if (removeAnimation_->state() != QAbstractAnimation::State::Running)
        {
            removeAnimation_->setDuration(isChat_ ? 200 : 0);
            removeAnimation_->start();
        }

        if (delCount && delCount == heads_.size())
        {
            if (headsAtBottom())
            {
                ensureHeightAnimationInitialized();
                if (heightAnimation_->state() != QAbstractAnimation::State::Running)
                {
                    heightAnimation_->stop();
                    heightAnimation_->setDirection(QAbstractAnimation::Backward);
                    heightAnimation_->start();
                }
            }
        }

        updateGeometry();
        update();
    }

    bool HistoryControlPageItem::areTheSameHeads(const QVector<Data::ChatHead>& _heads) const
    {
        return Data::isSameHeads(heads_, _heads);
    }

    void HistoryControlPageItem::updateSize()
    {
        resize(size());
    }

    void HistoryControlPageItem::setIsChat(bool _isChat)
    {
        isChat_ = _isChat;
    }

    void HistoryControlPageItem::showMessageStatus()
    {
        if (lastStatus_ == LastStatus::DeliveredToPeer || lastStatus_ == LastStatus::DeliveredToServer)
        {
            if (lastStatusAnimation_)
                lastStatusAnimation_->showStatus();
        }
    }


    void HistoryControlPageItem::hideMessageStatus()
    {
        if (lastStatus_ == LastStatus::DeliveredToPeer || lastStatus_ == LastStatus::DeliveredToServer)
        {
            if (lastStatusAnimation_)
                lastStatusAnimation_->hideStatus();
        }
    }
}
