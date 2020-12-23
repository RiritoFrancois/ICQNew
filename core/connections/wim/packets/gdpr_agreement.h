#pragma once

#include "../wim_packet.h"
#include "../../accept_agreement_info.h"

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
        class gdpr_agreement : public wim_packet
        {
        private:
            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual void parse_response_data_on_error(const rapidjson::Value& _data) override;
            virtual int32_t on_response_error_code() override;
            int32_t execute_request(const std::shared_ptr<core::http_request_simple>& request) override;
            virtual int32_t on_empty_data() override;

        public:
            gdpr_agreement(wim_packet_params _params, const accept_agreement_info& _info);
            virtual ~gdpr_agreement() = default;

            virtual priority_t get_priority() const override;
            virtual bool is_post() const override { return true; }
            virtual std::string_view get_method() const override;

        private:
            accept_agreement_info accept_info_;
        };
    }
}
