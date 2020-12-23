#pragma once

#include "../robusto_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}


namespace core
{
    namespace wim
    {
        class block_chat_member : public robusto_packet
        {
            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string aimid_;
            std::string contact_;
            bool block_;
            bool remove_messages_;

        public:

            block_chat_member(wim_packet_params _params, const std::string& _aimId, const std::string& _contact, bool _block, bool _remove_messages);
            virtual ~block_chat_member();

            const std::string& get_contact() const noexcept { return contact_; }
            const std::string& get_chat() const noexcept { return aimid_; }

            virtual std::string_view get_method() const override;
        };

    }

}
