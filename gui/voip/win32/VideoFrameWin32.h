#ifndef __VIDEO_FRAME_WIN32_H__
#define __VIDEO_FRAME_WIN32_H__

#include <QWidget>
#include "../VideoFrame.h"

namespace Ui
{
    class BaseVideoPanel;
}

namespace platform_win32
{
    void showOnCurrentDesktop(QWidget* _w, QWidget* _initiator, platform_specific::ShowCallback _callback);

    class GraphicsPanelWin32 : public platform_specific::GraphicsPanel
    {
        Q_OBJECT
    public:
        GraphicsPanelWin32(QWidget* _parent, std::vector<QPointer<Ui::BaseVideoPanel>>& _panels, bool primaryVideo);
        virtual ~GraphicsPanelWin32();
        void setOpacity(double _opacity) override;
    };
}

#endif//__VIDEO_FRAME_WIN32_H__
