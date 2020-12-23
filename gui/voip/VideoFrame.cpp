#include "stdafx.h"
#include "VideoFrame.h"
#include "CommonUI.h"

#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
#elif _WIN32
    #include "win32/VideoFrameWin32.h"
#elif __linux__
    #include "linux/VideoFrameLinux.h"
#else
    #error "video frame need to create"
#endif

platform_specific::GraphicsPanel* platform_specific::GraphicsPanel::create(
    QWidget* _parent,
    std::vector<QPointer<Ui::BaseVideoPanel>>& _panels, bool primaryVideo, bool titlePar)
{
#ifdef __APPLE__
    return  platform_macos::GraphicsPanelMacos::create(_parent, _panels, primaryVideo, titlePar);
#elif _WIN32
    return  new platform_win32::GraphicsPanelWin32(_parent, _panels, primaryVideo);
#elif __linux__
    return  new platform_linux::GraphicsPanelLinux(_parent, _panels, primaryVideo);
#else
    return nullptr;
#endif
}

WId platform_specific::GraphicsPanel::frameId() const
{
    return QWidget::winId();
}

void platform_specific::showInCurrentWorkspace(QWidget* _w, QWidget* _initiator, ShowCallback _callback)
{
#ifdef __APPLE__
    platform_macos::showInWorkspace(_w, std::move(_callback));
#elif _WIN32
    platform_win32::showOnCurrentDesktop(_w, _initiator, std::move(_callback));
#elif __linux__
    platform_linux::showOnCurrentDesktop(std::move(_callback));
#endif
}
