#pragma once

#include "controls/BackgroundWidget.h"

namespace Data
{
    class SmartreplySuggest;
}

namespace Emoji
{
    class EmojiCode;
}

namespace Ui
{
    using highlightsV = std::vector<QString>;

    class HistoryControl;
    class HistoryControlPage;
    class InputWidget;
    class DragOverlayWindow;
    class SmartReplyForQuote;

    enum class FrameCountMode;

    namespace Smiles
    {
        class SmilesMenu;
    }

    namespace Stickers
    {
        class StickersSuggest;
    }

    class ContactDialog : public BackgroundWidget
    {
        Q_OBJECT

    public Q_SLOTS:
        void onContactSelected(const QString& _aimId, qint64 _messageId, const highlightsV& _highlights, bool _ignoreScroll = false);
        void onContactSelectedToLastMessage(const QString& _aimId, qint64 _messageId);

        void onSmilesMenu(const bool _fromKeyboard = false);
        void onInputEditFocusOut();
        void onSendMessage(const QString& _contact);

        void emojiSelected(const Emoji::EmojiCode& _code, const QPoint _pos);

    private Q_SLOTS:
        void updateDragOverlay();
        void historyControlClicked();
        void inputTyped();
        void onQuotesAdded();
        void onQuotesDropped();
        void onSuggestShow(const QString& _text, const QPoint& _pos);
        void onSuggestHide();
        void suggestedStickerSelected(const QString& _stickerId);
        void onSmartrepliesForQuote(const std::vector<Data::SmartreplySuggest>& _suggests);
        void onMultiselectChanged();

    Q_SIGNALS:
        void contactSelected(const QString& _aimId, qint64 _messageId, const highlightsV& _highlights, bool _ignoreScroll = false) const;
        void contactSelectedToLastMessage(const QString& _aimId, qint64 _messageId) const;

        void sendMessage(const QString&) const;
        void clicked() const;

        void resized() const;

    private:
        HistoryControl* historyControlWidget_;
        InputWidget* inputWidget_;
        Smiles::SmilesMenu* smilesMenu_;
        DragOverlayWindow* dragOverlayWindow_;
        QTimer overlayUpdateTimer_;
        QStackedWidget* topWidget_;
        QMap<QString, QWidget*> topWidgetsCache_;
        Stickers::StickersSuggest* suggestsWidget_;
        FrameCountMode frameCountMode_;
        bool suggestWidgetShown_;
        SmartReplyForQuote* smartreplyForQuotePopup_;

        void initTopWidget();
        void initSmilesMenu();
        void initInputWidget();

        void updateDragOverlayGeometry();

        void insertTopWidget(const QString& _aimId, QWidget* _widget);
        void removeTopWidget(const QString& _aimId);

        void sendSuggestedStickerStats(const QString& _stickerId);

        struct SmartreplyForQuoteParams
        {
            QPoint pos_;
            QRect areaRect_;
            QSize maxSize_;
        };
        SmartreplyForQuoteParams getSmartreplyForQuoteParams() const;

        void sendSmartreplyForQuote(const Data::SmartreplySuggest& _suggest);

    public:
        explicit ContactDialog(QWidget* _parent);
        ~ContactDialog();

        void cancelSelection();
        void hideInput();

        void showDragOverlay();
        void hideDragOverlay();

        void hideSmilesMenu();

        HistoryControlPage* getHistoryPage(const QString& _aimId) const;
        Smiles::SmilesMenu* getSmilesMenu() const;

        void setFocusOnInputWidget();
        void setFocusOnInputFirstFocusable();
        bool canSetFocusOnInput() const;
        Ui::InputWidget* getInputWidget() const;
        bool inputHasSelection() const;

        void notifyApplicationWindowActive(const bool isActive);
        void notifyUIActive(const bool _isActive);

        bool isSuggestVisible() const;
        bool isEditingMessage() const;
        bool isShowingSmileMenu() const;
        bool isReplyingToMessage() const;
        bool isRecordingPtt() const;
        bool isPttRecordingHold() const;
        bool isPttRecordingHoldByKeyboard() const;
        bool isPttRecordingHoldByMouse() const;
        bool tryPlayPttRecord();
        bool tryPausePttRecord();

        void closePttRecording();
        void sendPttRecord();
        void stopPttRecord();
        void startPttRecordingLock();
        void dropReply();

        bool isPasteEnabled() const;

        void hideSuggest();
        Stickers::StickersSuggest* getStickersSuggests();

        void setFrameCountMode(FrameCountMode _mode);
        FrameCountMode getFrameCountMode() const;

        const QString& currentAimId() const;

        void hideSmartrepliesForQuoteAnimated();
        void hideSmartrepliesForQuoteForce();

    protected:
        void dragEnterEvent(QDragEnterEvent *) override;
        void dragLeaveEvent(QDragLeaveEvent *) override;
        void dragMoveEvent(QDragMoveEvent *) override;
        void resizeEvent(QResizeEvent*) override;
    };
}
