#pragma once
#include "RecentsTab.h"

namespace Logic
{
    class CustomAbstractListModel;
    class SearchModel;
    class SearchItemDelegate;
    enum class UpdateChatSelection;
    class CommonChatsModel;
    class CallsModel;
    class StatusProxyModel;
}

namespace Ui
{
    using highlightsV = std::vector<QString>;

    class FocusableListView;
    class SearchWidget;
    class ContextMenu;
    class Placeholder;

    class ContactListWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void searchEnd();
        void itemSelected(const QString& _aimid, qint64 _msgid, const highlightsV& _highlights);
        void itemClicked(const QString& _aimid);
        void groupClicked(int);
        void changeSelected(const QString&);
        void searchSuggestSelected(const QString& _pattern);
        void searchSuggestRemoved(const QString& _contact, const QString& _pattern);
        void clearSearchSelection();
        void forceSearchClear(const bool _force);

        //sidebar
        void selected(const QString&);
        void removeClicked(const QString&);
        void moreClicked(const QString&);
        void approve(const QString, bool);

    public:
        ContactListWidget(QWidget* _parent, const Logic::MembersWidgetRegim& _regim, Logic::CustomAbstractListModel* _chatMembersModel, Logic::AbstractSearchModel* _searchModel = nullptr, Logic::CommonChatsModel* _commonChatsModel = nullptr);
        ~ContactListWidget();

        void connectSearchWidget(SearchWidget* _widget);

        void installEventFilterToView(QObject* _filter);

        QWidget* indexWidget(int _index);
        void setIndexWidget(int index, QWidget* widget);

        void setClDelegate(Logic::AbstractItemDelegateWithRegim* _delegate);
        void setWidthForDelegate(int _width);
        void setDragIndexForDelegate(const QModelIndex& _index);
        void setPictureOnlyForDelegate(bool _value);

        void setEmptyIgnoreLabelVisible(bool _isVisible);
        void setSearchInDialog(const QString& _contact, bool _switchModel = true);
        bool getSearchInDialog() const;
        const QString& getSearchInDialogContact() const;

        bool isSearchMode() const;
        QString getSelectedAimid() const;

        Logic::MembersWidgetRegim getRegim() const;
        void setRegim(const Logic::MembersWidgetRegim _regim);

        void enableCLDelegateOpacity(bool _enable);
        bool isCLDelegateOpacityEnabled() const;

        Logic::AbstractSearchModel* getSearchModel() const;
        FocusableListView* getView() const;

        void triggerTapAndHold(bool _value);
        bool tapAndHoldModifier() const;

        void showSearch();

        void rewindToTop();

        void updateSelectedItem();

    public Q_SLOTS:
        void searchResult();
        void searchUpPressed();
        void searchDownPressed();
        void selectionChanged(const QModelIndex &);
        void select(const QString& _aimId);
        void select(const QString& _aimId, const qint64 _messageId, const highlightsV& _highlights, Logic::UpdateChatSelection _mode);
        void showContactsPopupMenu(const QString& aimId, bool _is_chat);
        void showCallsPopupMenu(Data::CallInfoPtr _call);

    private Q_SLOTS:
        void onItemClicked(const QModelIndex&);
        void onItemPressed(const QModelIndex&);
        void onMouseMoved(const QPoint& _pos, const QModelIndex& _index);
        void onMouseWheeled();
        void onMouseWheeledStats();
        void onSearchResults();
        void onSearchSuggests();
        void searchResults(const QModelIndex &, const QModelIndex &);
        void searchClicked(const QModelIndex& _current);
        void showPopupMenu(QAction* _action);

        void onSearchInputCleared();
        void searchPatternChanged(const QString&);
        void onDisableSearchInDialog();
        void touchScrollStateChanged(QScroller::State _state);

        void scrollToCategory(const SearchCategory _category);
        void scrollToItem(const QString& _aimId);

        void removeSuggestPattern(const QString& _contact, const QString& _pattern);

        void showPlaceholder();
        void hidePlaceholder();
        void showNoSearchResults();
        void hideNoSearchResults();
        void showSearchSpinner();
        void hideSearchSpinner();

        void scrolled(const int _value);

    private:
        void switchToInitial(bool _initial);
        void searchUpOrDownPressed(bool _isUp);
        void setKeyboardFocused(bool _isFocused);
        void initSearchModel(Logic::AbstractSearchModel* _searchModel);
        bool isSelectMembersRegim() const;
        void ensureScrollToItemAnimationInitialized();

        void selectCurrentSearchCategory();

        void sendGlobalSearchResultStats();

        using SearchHeaders = std::vector<std::pair<SearchCategory, QModelIndex>>;
        SearchHeaders getCurrentCategories() const;

    private:
        QVBoxLayout* layout_;
        QVBoxLayout* viewLayout_;
        FocusableListView* view_;

        EmptyIgnoreListLabel* emptyIgnoreListLabel_;
        DialogSearchViewHeader* dialogSearchViewHeader_;
        GlobalSearchViewHeader* globalSearchViewHeader_;
        QVariantAnimation* scrollToItemAnim_;

        std::string scrollStatWhere_;
        QTimer* scrollStatsTimer_;

        Logic::AbstractItemDelegateWithRegim* clDelegate_;
        Logic::AbstractItemDelegateWithRegim* searchDelegate_;

        Placeholder* contactsPlaceholder_;
        QWidget* noSearchResults_;
        QWidget* searchSpinner_;
        QWidget* viewContainer_;

        Logic::MembersWidgetRegim regim_;
        Logic::CustomAbstractListModel* chatMembersModel_;
        Logic::AbstractSearchModel* searchModel_;
        Logic::ContactListWithHeaders* clModel_;
        Logic::CommonChatsModel* commonChatsModel_;
        Logic::CallsModel* callsModel_;

        ContextMenu* popupMenu_;

        QString searchDialogContact_;
        bool noSearchResultsShown_;
        bool searchResultsRcvdFirst_;
        bool searchResultsStatsSent_;
        bool initial_;
        bool tapAndHold_;
    };
}
