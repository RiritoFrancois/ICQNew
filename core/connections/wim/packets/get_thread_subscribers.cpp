#include "stdafx.h"
#include "get_thread_subscribers.h"

#include "../../../http_request.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

get_thread_subscribers::get_thread_subscribers(wim_packet_params _params, std::string_view _thread_id, std::string_view _role, uint32_t _page_size, std::string_view _cursor)
    : robusto_packet(std::move(_params))
    , thread_id_(_thread_id)
    , role_(_role)
    , cursor_(_cursor)
    , page_size_(_page_size)
    , reset_pages_(false)
{
}

get_thread_subscribers::~get_thread_subscribers()
{
}

std::string_view core::wim::get_thread_subscribers::get_method() const
{
    return "thread/subscribers/get";
}

int core::wim::get_thread_subscribers::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t core::wim::get_thread_subscribers::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("threadId", thread_id_, a);

    if (cursor_.empty())
        node_params.AddMember("pageSize", page_size_, a);
    else
        node_params.AddMember("cursor", cursor_, a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t core::wim::get_thread_subscribers::parse_results(const rapidjson::Value& _node_results)
{
    if (get_status_code() == robusto_protocol_error::reset_page)
        reset_pages_ = true;

    if (result_.unserialize(_node_results) != 0)
        return wpie_http_parse_response;
    result_.set_id(thread_id_);

    return 0;
}

int32_t core::wim::get_thread_subscribers::on_response_error_code()
{
    // TODO: handle
    return robusto_packet::on_response_error_code();
}

bool core::wim::get_thread_subscribers::is_status_code_ok() const
{
    return get_status_code() == robusto_protocol_error::reset_page || robusto_packet::is_status_code_ok();
}
