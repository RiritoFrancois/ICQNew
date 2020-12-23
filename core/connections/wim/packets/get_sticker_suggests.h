#pragma once

#include "../robusto_packet.h"
#include "../../../stickers/suggests.h"

namespace core
{
    namespace tools
    {
        class binary_stream;
        class http_request_simple;
    }

    namespace smartreply
    {
        class suggest;
        enum class type;
    }

    namespace wim
    {
        class get_sticker_suggests : public robusto_packet
        {
            std::string aimid_;
            std::string text_;
            std::vector<std::string> stickers_;

            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_results(const rapidjson::Value& _data) override;

        public:
            get_sticker_suggests(wim_packet_params _params, std::string _aimid, std::string _text);
            virtual ~get_sticker_suggests() = default;

            const std::vector<std::string>& get_stickers() const { return stickers_; }
            const std::string& get_contact() const { return aimid_; }
            virtual std::string_view get_method() const override;
        };
    }
}
