#pragma once

#include "Common.h"

#include "../../types/search_result.h"
#include "../../controls/TextUnit.h"

namespace Logic
{
    class SearchItemDelegate: public AbstractItemDelegateWithRegim
    {
        Q_OBJECT

    public Q_SLOTS:
        void onContactSelected(const QString& _aimId, qint64 _msgId);
        void clearSelection();
        void dropCache();

    public:
        SearchItemDelegate(QObject* _parent);

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        void setRegim(int _regim) override;
        void setFixedWidth(int width) override;
        void blockState(bool value) override;
        void setDragIndex(const QModelIndex& index) override;

        QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;
        bool setHoveredSuggetRemove(const QModelIndex & _itemIndex);

        bool needsTooltip(const QString& _aimId, const QModelIndex& _index, QPoint _posCursor = {}) const override;

    private:
        void drawContactOrChatItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, const bool _isSelected) const;
        void drawMessageItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::MessageSearchResultSptr& _item, const bool _isSelected) const;
        void drawServiceItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::ServiceSearchResultSptr& _item) const;
        void drawSuggest(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::SuggestSearchResultSptr& _item, const QModelIndex& _index, const bool _isHovered) const;

        struct CachedItem
        {
            int cachedWidth_ = 0;
            bool isSelected_ = false;
            bool isOfficial_ = false;

            Data::AbstractSearchResultSptr searchResult_ = nullptr;
            Ui::TextRendering::TextUnitPtr name_ = nullptr;
            Ui::TextRendering::TextUnitPtr nick_ = nullptr;
            Ui::TextRendering::TextUnitPtr text_ = nullptr;
            Ui::TextRendering::TextUnitPtr time_ = nullptr;

            void draw(QPainter& _painter);
        };
        using ContactsMap = std::unordered_map<QString, std::unique_ptr<CachedItem>, Utils::QStringHasher>;
        using MessagesMap = std::unordered_map<qint64, std::unique_ptr<CachedItem>>;

        CachedItem* getCachedItem(const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, const bool _isSelected) const;
        CachedItem* drawCachedItem(QPainter& _painter, const QStyleOptionViewItem& _option, const Data::AbstractSearchResultSptr& _item, const bool _isSelected) const;

        void fillContactItem(CachedItem& ci, const QStyleOptionViewItem& _option, const int _maxTextWidth) const;
        void fillMessageItem(CachedItem& ci, const QStyleOptionViewItem& _option, const int _maxTextWidth) const;
        void reelideMessage(CachedItem& ci, const QStyleOptionViewItem& _option, const int _maxTextWidth) const;

        std::pair<bool, bool> getMouseState(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;

        void drawAvatar(QPainter& _painter, const Ui::ContactListParams& _params, const QString& _aimid, const int _itemHeight, const bool _isSelected, const bool _isMessage) const;

        QString getSenderName(const Data::MessageBuddySptr& _msg, const bool _fromDialogSearch) const;

        mutable ContactsMap contactCache_;
        mutable MessagesMap messageCache_;

        qint64 selectedMessage_;

        mutable QTime lastDrawTime_;
        QTimer cacheTimer_;

        QModelIndex hoveredRemoveSuggestBtn_;
    };
}
