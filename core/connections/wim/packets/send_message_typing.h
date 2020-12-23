#ifndef __WIM_SEND_MESSAGE_TYPING_H_
#define __WIM_SEND_MESSAGE_TYPING_H_

#pragma once

#include "../wim_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}

namespace core
{
    enum class typing_status;

    namespace wim
    {
        class send_message_typing: public wim_packet
        {
            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            const std::string contact_;
            const std::string id_;
            const core::typing_status status_;

        public:

            send_message_typing(wim_packet_params _params, const std::string& _contact, const core::typing_status& _statusm, const std::string& _id);
            virtual ~send_message_typing();

            virtual std::string_view get_method() const override;
        };

    }

}

#endif // __WIM_SEND_MESSAGE_TYPING_H_
