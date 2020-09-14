#include "stdafx.h"
#include "get_chat_members.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;

get_chat_members::get_chat_members(wim_packet_params _params, std::string_view _aimid, std::string_view _role, uint32_t _page_size, std::string_view _cursor)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
    , role_(_role)
    , cursor_(_cursor)
    , page_size_(_page_size)
    , reset_pages_(false)
{
}

get_chat_members::~get_chat_members() = default;

int32_t get_chat_members::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("id", aimid_, a);
    node_params.AddMember("pageSize", page_size_, a);

    if (cursor_.empty())
    {
        rapidjson::Value filter_params(rapidjson::Type::kObjectType);
        filter_params.AddMember("role", role_, a);
        node_params.AddMember("filter", std::move(filter_params), a);
    }
    else
    {
        node_params.AddMember("cursor", cursor_, a);
    }

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "getChatMembers");

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t core::wim::get_chat_members::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    auto size = _response->available();
    load_response_str((const char*)_response->read(size), size);
    try
    {
        rapidjson::Document doc;
        if (doc.Parse(response_str().c_str()).HasParseError())
            return wpie_error_parse_response;

        auto iter_status = doc.FindMember("status");
        if (iter_status == doc.MemberEnd())
            return wpie_error_parse_response;

        auto iter_code = iter_status->value.FindMember("code");
        if (iter_code == iter_status->value.MemberEnd())
            return wpie_error_parse_response;

        status_code_ = iter_code->value.GetUint();

        if (status_code_ == 20000 || status_code_ == 20002)
        {
            if (status_code_ == 20002)
                reset_pages_ = true;

            if (const auto iter_result = doc.FindMember("results"); iter_result != doc.MemberEnd())
                return parse_results(iter_result->value);
        }
        else
        {
            return on_response_error_code();
        }
    }
    catch (...)
    {
        return 0;
    }

    return 0;
}

int32_t get_chat_members::parse_results(const rapidjson::Value& _node_results)
{
     if (result_.unserialize(_node_results) != 0)
         return wpie_http_parse_response;

    return 0;
}

int32_t get_chat_members::on_response_error_code()
{
    if (status_code_ == 40001)
        return wpie_error_robusto_you_are_not_chat_member;
    else if (status_code_ == 40002)
        return wpie_error_robusto_you_are_blocked;

    return robusto_packet::on_response_error_code();
}
