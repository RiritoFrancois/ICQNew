#pragma once

#include "../../../common.shared/message_processing/message_tokenizer.h"

#include "../history/Message.h"

namespace Ui
{
    namespace ComplexMessage
    {
        struct TextChunk
        {
            enum class Type
            {
                Invalid,

                Min,

                Undefined,
                Text,
                FormattedText,
                GenericLink,
                ImageLink,
                Junk,
                FileSharingImage,
                FileSharingImageSticker,
                FileSharingGif,
                FileSharingGifSticker,
                FileSharingVideo,
                FileSharingVideoSticker,
                FileSharingLottieSticker,
                FileSharingPtt,
                FileSharingGeneral,
                FileSharingUpload,
                Sticker,
                ProfileLink,

                Max
            };

            static const TextChunk Empty;

            TextChunk();
            TextChunk(Type _type, QString _text, QString _imageType = {}, int32_t _durationSec = -1);
            TextChunk(const Data::FormattedStringView& _view, Type _type = Type::FormattedText);

            int32_t length() const;

            TextChunk mergeWith(const TextChunk& chunk) const;


            QStringView getPlainText() const { return formattedText_.isEmpty() ? text_ : formattedText_.string(); }

            const Data::FormattedStringView& getText() const { return formattedText_; }

            Type Type_;

            Data::FormattedStringView formattedText_;

            QString text_;

            QString ImageType_;

            int32_t DurationSec_;

            HistoryControl::StickerInfoSptr Sticker_;

            HistoryControl::FileSharingInfoSptr FsInfo_;
        };

        using FixedUrls = std::vector<common::tools::url_parser::compare_item>;

        class ChunkIterator final
        {
        public:
            explicit ChunkIterator(const QString& _text);

            ChunkIterator(const QString& _text, FixedUrls&& _urls);
            ChunkIterator(const Data::FormattedString& _text, FixedUrls&& _urls);

            bool hasNext() const;
            TextChunk current(bool _allowSnippet = true, bool _forcePreview = false) const;
            void next();

        private:
            common::tools::message_tokenizer tokenizer_;
            Data::FormattedStringView formattedText_;
        };
    }
}
