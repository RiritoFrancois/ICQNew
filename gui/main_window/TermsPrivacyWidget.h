#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include "namespaces.h"

#if defined(__APPLE__)
class MacMenuBlocker;
#endif

namespace Ui
{
    class PictureWidget;
    class TextBrowserEx;
    class DialogButton;

    const QString& legalTermsUrl();
    const QString& privacyPolicyUrl();

    class TermsPrivacyWidget : public QWidget
    {
        Q_OBJECT

    public:
        struct Options
        {
            bool isGdprUpdate_ = false;
        };

    public:
        TermsPrivacyWidget(const QString& _titleHtml,
            const QString& _descriptionHtml,
            const Options& _options,
            QWidget* _parent = nullptr);
        ~TermsPrivacyWidget();

    Q_SIGNALS:
        void agreementAccepted();

    public Q_SLOTS:
        void onAgreeClicked();
        void reportAgreementAccepted();

    protected:
        void keyPressEvent(QKeyEvent *_event) override;
        void showEvent(QShowEvent *_event) override;

    private:
        void onAnchorClicked(const QUrl& _link) const;

    private:
        QPixmap iconPixmap_;

        PictureWidget* icon_ = nullptr;
        TextBrowserEx* title_ = nullptr;
        TextBrowserEx* description_ = nullptr;
        DialogButton* agreeButton_ = nullptr;

        QVBoxLayout* layout_ = nullptr;

        Options options_;

#if defined(__APPLE__)
        std::unique_ptr<MacMenuBlocker> macMenuBlocker_;
#endif
    };

}
