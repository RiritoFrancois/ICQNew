#pragma once
#include "VideoFrame.h"
#include "DetachedVideoWnd.h"
#ifdef __APPLE__
#include "macos/VideoFrameMacos.h"
#endif
#include "media/permissions/MediaCapturePermissions.h"

namespace voip_manager
{
    struct CipherState;
    struct FrameSize;
}

namespace Ui
{
    class DetachedVideoWindow;
    class ShadowWindow;
    class VoipSysPanelHeader;
    class VideoPanel;
    class MaskPanel;
    //class SecureCallWnd;
    class VideoWindowHeader;
    class TextEmojiWidget;

    class GridControlPanel;
    class MicroAlert;
    enum class MicroIssue;

    class VideoWindowHeader : public Ui::MoveablePanel
    {
        Q_OBJECT

    public:

        VideoWindowHeader(QWidget* parent);

        bool uiWidgetIsActive() const override;
        void updatePosition(const QWidget& parent) override;
        void setTitle(const QString& text);

    Q_SIGNALS:

        void onMinimized();
        void onFullscreen();

    protected:

        void mouseDoubleClickEvent(QMouseEvent * e) override;


    private:
        Ui::TextEmojiWidget* title_;
    };

    class VideoWindow : public QWidget
    {
        Q_OBJECT

    private:
        typedef platform_specific::GraphicsPanel FrameControl_t;

    protected:
        bool nativeEvent(const QByteArray& _data, void* _message, long* _result) override;
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;
        void resizeEvent(QResizeEvent* _e) override;
        void closeEvent(QCloseEvent *) override;
        void paintEvent(QPaintEvent *_e) override;
        void changeEvent(QEvent *) override;
        void escPressed();
        void keyPressEvent(QKeyEvent* _e) override;
        void keyReleaseEvent(QKeyEvent* _e) override;

        void executeCommandList();
        void setWindowTitle(const QString& text);

#ifndef __APPLE__
        void mouseDoubleClickEvent(QMouseEvent* _e) override;
#endif
#ifndef _WIN32
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent * event) override;
        void mousePressEvent(QMouseEvent * event) override;
        void wheelEvent(QWheelEvent * event) override;
        void moveEvent(QMoveEvent * event) override;
        bool event(QEvent* _event) override;

        // @return true if we resend message to any transparent panel
        template <typename E> bool resendMouseEventToPanel(E* event_);
#endif

    private:
        quintptr getContentWinId();
        void updatePanels() const;
        void _switchFullscreen();
        void checkPanelsVisibility(bool _forceHide = false);
        inline void updateWindowTitle();
        bool isOverlapped() const;

        // @return true, if we were able to load size from settings
        bool getWindowSizeFromSettings(int& _nWidth, int& _nHeight);

        void offsetWindow(int _bottom, int _top);
        void updateUserName();

        // We use this proxy to catch call methods during fullscreen animation
        // we will save commands and call it later, after fullscreen animation
        void callMethodProxy(const QString& _method);

        void fadeInPanels(int kAnimationDefDuration);
        void fadeOutPanels(int kAnimationDefDuration);
        void hidePanels();

        // call this method in all cases, when you need to hide panels
        void tryRunPanelsHideTimer();
        //void hideSecurityDialog();

        bool hasRemoteVideoInCall(); // @return true for calls (not for conference), if remote camera is turned on.
        bool hasRemoteVideoForConference(); // @return true for conference, if any remote camera is turned on.

        void removeUnneededRemoteVideo(); // Remove unneeded elements from hasRemoteVideo_
        //void checkCurrentAspectState();

        void updateOutgoingState(const voip_manager::ContactEx& _contactEx);
        //void updateTopPanelSecureCall();

        void createdNewCall();
        void sendStatistic();

        void updateMicroAlertState();
        void checkMicroPermission();
        void showPermissionPanel();

    private Q_SLOTS:
        void checkOverlap();
        void checkPanelsVis();
        void onVoipMouseTapped(quintptr, const std::string& _tapType, const std::string&);
        void onVoipCallNameChanged(const voip_manager::ContactsList&);
        void onVoipCallTimeChanged(unsigned _secElapsed, bool _hasCall);
        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);
        void onVoipCallConnected(const voip_manager::ContactEx& _contactEx);
        void onVoipMediaRemoteVideo(const voip_manager::VideoEnable& videoEnable);
        void onVoipWindowRemoveComplete(quintptr _winId);
        void onVoipWindowAddComplete(quintptr _winId);
        void onVoipCallOutAccepted(const voip_manager::ContactEx& _contactEx);
        void onVoipCallCreated(const voip_manager::ContactEx& _contactEx);
        //void onVoipChangeWindowLayout(intptr_t hwnd, bool bTray, const std::string& layout);
        void onVoipMainVideoLayoutChanged(const voip_manager::MainVideoLayout& mainLayout);

        void onEscPressed();

        void onPanelClickedMinimize();
        void onPanelClickedMaximize();

        void onPanelMouseEnter();
        void onPanelMouseLeave();

        //void onVoipUpdateCipherState(const voip_manager::CipherState& _state);
        //void onSecureCallClicked(const QRect&);
        //void onSecureCallWndOpened();
        //void onSecureCallWndClosed();

        // These methods are used under macos to close video window correctly from the fullscreen mode
        void fullscreenAnimationStart();
        void fullscreenAnimationFinish();
        void activeSpaceDidChange();
        void windowWillDeminiaturize();
        void windowDidDeminiaturize();

        void onScreenSharing();
        void onSetPreviewPrimary();
        void onSetContactPrimary();
        void onSetContactPrimary(const std::string& contact);

        void onShowMaskList();
        void onHideMaskList();

        void getCallStatus(bool& isAccepted);

        void onMaskListAnimationFinished(bool out);

        void updateConferenceMode(voip_manager::VideoLayout layout);
        void onAddUserClicked();
        void onShowMaskPanel();
        void onHideMaskPanel();
        void onCloseActiveMask();

        void setShowMaskPanelState();
        void setHideMaskPanelState();

        void resizeToDefaultSize();

        void showScreenPermissionsPopup(media::permissions::DeviceType type);

        void onInviteVCSUrl(const QString& _url);

        void showMicroPermissionPopup();

    public:
        VideoWindow();
        ~VideoWindow();

        void hideFrame();
        void showFrame();
        bool isActiveWindow() const;
        bool isInFullscreen() const;
        bool isMinimized() const;
        VideoPanel* getVideoPanel() const;
        void showToast(const QString& _text, int _maxLineCount = 1);
        void setMicrophoneAlert(MicroIssue _issue);
        void updateHangUpState();
        void raiseWindow();

    Q_SIGNALS:

        void finished();
        void aboutToHide();

    private:
        FrameControl_t* rootWidget_;

        // Video window panels list
        std::vector<QPointer<BaseVideoPanel>> panels_;

#ifndef _WIN32
        // Transparent widgets. We resend mouse events to these widgets under macos,
        // because we did not find any better way to catch mouse events for tranparent panels
        std::vector<QPointer<BaseVideoPanel>> transparentPanels_;
#endif

        //std::unique_ptr<VoipSysPanelHeader> topPanelOutgoing_;
        std::unique_ptr<GridControlPanel>   gridButtonPanel_;
        std::unique_ptr<MicroAlert>         microAlert_;
        std::unique_ptr<MaskPanel>          maskPanel_;
        std::unique_ptr<VideoPanel>         videoPanel_;
        std::unique_ptr<TransparentPanel>   transparentPanelOutgoingWidget_;

        std::unique_ptr<DetachedVideoWindow> detachedWnd_;
        ResizeEventFilter*     eventFilter_;

        QTimer checkOverlappedTimer_;
        QTimer showPanelTimer_;

        // Remote video by users.
        QHash <QString, bool> hasRemoteVideo_;
        bool outgoingNotAccepted_;

        // Current contacts
        std::vector<voip_manager::Contact> currentContacts_;

        //SecureCallWnd* secureCallWnd_;

        struct
        {
            std::string name;
            unsigned time;
        } callDescription;

        ShadowWindowParent shadow_;

        // Last applied offsets.
        int lastBottomOffset_;
        int lastTopOffset_;

        // All is connected and talking now.
        bool startTalking;
        bool miniWindowShown = false;

        // true for security calls.
        //bool enableSecureCall_;

        enum SHOW_MASK_PANEL {SMP_HIDE, SMP_ANIMATION, SMP_SHOW};
        SHOW_MASK_PANEL maskPanelState;

        voip_manager::MainVideoLayout currentLayout_;

        // Window header size.
        int headerHeight_ = 0;

        struct ButtonStatisticData;

        std::unique_ptr<ButtonStatisticData> buttonStatistic_;

#ifdef __APPLE__
        // We use fullscreen notification to fix a problem on macos
        platform_macos::FullScreenNotificaton fullscreenNotification_;
        // Commands after fullscreen mode
        QList<QString> commandList_;
        bool isFullscreenAnimation_;
        bool changeSpaceAnimation_;

        // We have own double click processing,
        // Because Qt has bug with double click and fullscreen
        QTime doubleClickTime_;
        QPoint doubleClickMousePos_;
#endif
#ifdef _WIN32
        VideoWindowHeader* header_;
#endif

        MicroIssue microIssue_;
        QPointer<media::permissions::PermissonsChangeNotifier> permissonsChangeNotifier_;
    };
}
