#pragma once
#include "../../core/Voip/VoipManagerDefines.h"
#include <memory>
#ifdef __APPLE__
#include "macos/VideoFrameMacos.h"
#endif

namespace voip_manager
{
    struct CipherState;
    struct FrameSize;
}

namespace Ui
{
    class BaseVideoPanel;
    class ShadowWindow;

    class ResizeEventFilter : public QObject
    {
        Q_OBJECT

    public:
        ResizeEventFilter(std::vector<QPointer<BaseVideoPanel>>& panels, ShadowWindow* shadow, QObject* _parent);

        void addPanel(BaseVideoPanel* _panel);
        void removePanel(BaseVideoPanel* _panel);

    protected:
        bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        std::vector<QPointer<BaseVideoPanel>> panels_;
        ShadowWindow* shadow_;
    };

    class ShadowWindowParent
    {
    public:
        ShadowWindowParent(QWidget* parent);
        ~ShadowWindowParent();

        void showShadow();
        void hideShadow();
        ShadowWindow* getShadowWidget();
        void setActive(bool _value);

    protected:
        // shadow
        ShadowWindow* shadow_;
    };

    struct UIEffects
    {
        UIEffects(QWidget& _obj, bool opacity = true, bool geometry = true);
        virtual ~UIEffects();

        void fadeOut(unsigned _interval);
        void fadeIn(unsigned _interval);

        void geometryTo(const QRect& _rc, unsigned _interval);
        void geometryTo(const QRect& _rcStart, const QRect& _rcFinish, unsigned _interval);
        bool isFadedIn();
        bool isGeometryRunning();
        void forceFinish();

    private:
        bool fadedIn_;

        QGraphicsOpacityEffect* fadeEffect_;
        std::unique_ptr<QPropertyAnimation> animation_;
        std::unique_ptr<QPropertyAnimation> resizeAnimation_;

        QWidget& obj_;
    };

    class SharingScreenFrame;
    // Inherit from this class to create panel in voip window.
    class BaseVideoPanel : public QWidget
    {
        Q_OBJECT
    public:
        BaseVideoPanel(QWidget* parent, Qt::WindowFlags f = Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

        virtual void updatePosition(const QWidget& parent) = 0;

        virtual void fadeIn(unsigned int duration);
        virtual void fadeOut(unsigned int duration);
        virtual void forceFinishFade();

        bool isGrabMouse();
        virtual bool isFadedIn();

        void keyPressEvent(QKeyEvent* _event) override;
        void keyReleaseEvent(QKeyEvent* _event) override;

    protected:

        bool grabMouse;

    private:
        std::unique_ptr<UIEffects> effect_;
    };

    // Inherit from this class to create panel on top in voip window.
    class BaseTopVideoPanel : public BaseVideoPanel
    {
        Q_OBJECT
    public:
#if defined(__linux__)
        BaseTopVideoPanel(QWidget* parent, Qt::WindowFlags f = Qt::Widget);
#else
        BaseTopVideoPanel(QWidget* parent, Qt::WindowFlags f = Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
#endif
        void updatePosition(const QWidget& parent) override;

        void setVerticalShift(int _shift);

    private:
        int verShift_ = 0;
    };

    // Inherit from this class to create panel on bottom in voip window.
    class BaseBottomVideoPanel : public BaseVideoPanel
    {
        Q_OBJECT
    public:
        BaseBottomVideoPanel(QWidget* parent,
#ifndef __linux__
          Qt::WindowFlags f = Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint
#else
          Qt::WindowFlags f = Qt::Widget
#endif
                );

        void updatePosition(const QWidget& parent) override;
    };

    // This panel fill all parent window
    class FullVideoWindowPanel : public Ui::BaseVideoPanel
    {
        Q_OBJECT

    public:

        FullVideoWindowPanel(QWidget* parent);

        virtual void updatePosition(const QWidget& parent) override;

        virtual void fadeIn(unsigned int duration)  override {}
        virtual void fadeOut(unsigned int duration) override {}

    Q_SIGNALS:

        void onResize();
        void aboutToHide();

    protected:

        void resizeEvent(QResizeEvent *event) override;
    };

    // Use this class if you want to process
    // mouse event on transparent panels.
    class PanelBackground : public QWidget
    {
        Q_OBJECT
    public:
        PanelBackground(QWidget* parent);

        void updateSizeFromParent();
    private:
        std::unique_ptr<UIEffects> videoPanelEffect_;
    };


    /**
     * This widget is used to process mouse event for another panel if it has transparent pixels.
     * Under Windows it is imposible to get mouse event for tranparent part of widgets.
     * We have outgoing panel which implements drag & drop, but it has trasporent parts.
     */
    class TransparentPanel : public BaseVideoPanel
    {
        Q_OBJECT

    public:
        TransparentPanel(QWidget* _parent, BaseVideoPanel* _eventWidget);
        ~TransparentPanel();

        void updatePosition(const QWidget& parent) override;

        virtual void fadeIn(unsigned int duration) override {}
        virtual void fadeOut(unsigned int duration) override {}

    protected:
        void resizeEvent(QResizeEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void resendMouseEventToPanel(QMouseEvent *event);

    protected:
        // We will resend events to this widget.
        BaseVideoPanel* eventWidget_;
        // Background widget to able process mouse events on transparent parts of panel.
        PanelBackground* backgroundWidget_;
    };

    class MoveablePanel : public BaseTopVideoPanel
    {
        Q_OBJECT
    public:
        MoveablePanel(QWidget* _parent);
        MoveablePanel(QWidget* _parent, Qt::WindowFlags f);
        virtual ~MoveablePanel();

    Q_SIGNALS:
        void onkeyEscPressed();

    private:
        QWidget* parent_;
        struct
        {
            QPoint posDragBegin;
            bool isDraging;
        } dragState_;

        void changeEvent(QEvent*) override;
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;
        void keyReleaseEvent(QKeyEvent*) override;

    protected:
        virtual bool uiWidgetIsActive() const = 0;
    };

    // Show dialog to add new users to video call.
    void showAddUserToVideoConverenceDialogVideoWindow(QObject* parent, FullVideoWindowPanel* parentWindow);
    void showAddUserToVideoConverenceDialogMainWindow(QObject* parent, QWidget* parentWindow);

    void showInviteVCSDialogVideoWindow(FullVideoWindowPanel* _parentWindow, const QString& _url);
}
