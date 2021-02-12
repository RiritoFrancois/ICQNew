#pragma once

#include "GenericBlock.h"
#include "controls/textrendering/TextRendering.h"

UI_NS_BEGIN

namespace TextRendering
{
    class TextUnit;
}

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

enum class BlockSelectionType;
class TextBlockLayout;

class TextBlock : public GenericBlock
{
    friend class TextBlockLayout;

    Q_OBJECT

Q_SIGNALS:
    void selectionChanged();

public:
    TextBlock(ComplexMessageItem *_parent, const QString& _text, const Ui::TextRendering::EmojiSizeType _emojiSizeType = Ui::TextRendering::EmojiSizeType::ALLOW_BIG);

    virtual ~TextBlock() override;

    void clearSelection() override;

    IItemBlockLayout* getBlockLayout() const override;

    QString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;

    bool updateFriendly(const QString& _aimId, const QString& _friendly) override;

    bool isDraggable() const override;

    bool isSelected() const override;

    bool isAllSelected() const override;

    bool isSharingEnabled() const override;

    void selectByPos(const QPoint& from, const QPoint& to, bool) override;

    void selectAll() override;

    MenuFlags getMenuFlags(QPoint p) const override;

    ContentType getContentType() const override { return ContentType::Text; }

    bool isBubbleRequired() const override;

    bool pressed(const QPoint& _p) override;

    bool clicked(const QPoint& _p) override;

    void doubleClicked(const QPoint& _p, std::function<void(bool)> _callback = std::function<void(bool)>()) override;

    QString linkAtPos(const QPoint& pos) const override;

    std::optional<QString> getWordAt(QPoint) const override;

    bool replaceWordAt(const QString&, const QString&, QPoint) override;

    QString getTrimmedText() const override;

    int desiredWidth(int _width = 0) const override;

    QString getTextForCopy() const override;

    QString getTextInstantEdit() const override;

    bool getTextStyle() const;

    void releaseSelection() override;

    bool isOverLink(const QPoint& _mousePosGlobal) const override;

    void setText(const QString& _text) override;

    void setEmojiSizeType(const TextRendering::EmojiSizeType _emojiSizeType) override;

    void highlight(const highlightsV& _hl) override;
    void removeHighlight() override;

    bool managesTime() const override { return true; }

    void startSpellChecking() override;

    int effectiveBlockWidth() const override { return desiredWidth(); }

    void setSpellErrorsVisible(bool _visible) override;

protected:
    void drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor) override;

    void initialize() override;

    void mouseMoveEvent(QMouseEvent *e) override;

    void leaveEvent(QEvent *e) override;

private:
    void reinit();
    void initTextUnit();
    void initTripleClickTimer();

    void onTextUnitChanged();

    void adjustEmojiSize();

    void updateStyle() override;
    void updateFonts() override;

    QPoint mapPoint(const QPoint& _complexMsgPoint) const;

    void startSpellCheckingIfNeeded();

private:
    TextBlockLayout* Layout_;

    std::unique_ptr<TextRendering::TextUnit> textUnit_;

    QTimer* TripleClickTimer_ = nullptr;
    TextRendering::EmojiSizeType emojiSizeType_;
    bool needSpellCheck_ = false;
};

UI_COMPLEX_MESSAGE_NS_END
