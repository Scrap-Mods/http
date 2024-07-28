#include "dllmain.hpp"

/*
    local sm_http = require("sm_http")

    sm_http.request("google.com", function(data, url, status, headers)
        print(data)
    end)
*/

int tick(lua_State*) {
    for (size_t i = requests->size(); i-- > 0;) {
        auto& request = requests->at(i);
        lua_State* L = request.m_state;

        if (request.m_future_response.wait_for(std::chrono::seconds(0)) != std::future_status::ready) continue;
            
        // call callback(data, url, status, headers)
        cpr::Response response = request.m_future_response.get();

        lua_rawgeti(L, LUA_REGISTRYINDEX, request.m_callbackRef);

        lua_pushstring(L, response.text.c_str());
        lua_pushstring(L, response.url.c_str());
        lua_pushnumber(L, response.status_code);

        lua_createtable(L, 0, 5);

        for (const auto& header : response.header) {
            lua_pushstring(L, header.second.c_str());
            lua_setfield(L, -2, header.first.c_str());
        }

        lua_call(L, 4, 0);

        luaL_unref(L, LUA_REGISTRYINDEX, request.m_callbackRef);

        requests->erase(requests->begin() + i); // pop the request
	}

	return 0;
}

void pushResponse(lua_State* L, int narg, cpr::AsyncResponse&& m_future_response) {
    lua_pushvalue(L, narg);

    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    requests->emplace_back(std::move(m_future_response), ref, L);
}

void genericGetRequest(lua_State* L, getRquestEnum method) {
    lua_checkargs(L, 2, true);

    std::shared_ptr<cpr::Session> session = std::make_shared<cpr::Session>();

    size_t size;
    const char* url = luaL_checklstring(L, 1, &size);

    session->SetUrl(cpr::Url(url, size));
    session->SetTimeout(cpr::Timeout{ 60 * 1000 });

    luaL_checktype(L, 2, LUA_TFUNCTION);

    cpr::Header headers = getDefaultHeader();
    if (lua_gettop(L) == 3) { // [headers]
        headers = lua_checkheaders(L, 3);
    }

    session->SetHeader(headers);

    switch (method) {
    case getRquestEnum::get:
        pushResponse(L, 2, std::move(session->GetAsync()));

        break;
    case getRquestEnum::del:
        pushResponse(L, 2, std::move(session->DeleteAsync()));

        break;
    case getRquestEnum::head:
        pushResponse(L, 2, std::move(session->HeadAsync()));

        break;
    case getRquestEnum::options:
        pushResponse(L, 2, std::move(session->OptionsAsync()));

        break;
    default:
        assert();
        return;
    }
}

void genericPostRequest(lua_State* L, postRquestEnum method) {
    lua_checkargs(L, 3, true);

    std::shared_ptr<cpr::Session> session = std::make_shared<cpr::Session>();

    size_t size;
    const char* url = luaL_checklstring(L, 1, &size);

    session->SetUrl(cpr::Url(url, size));
    session->SetTimeout(cpr::Timeout{ 60 * 1000 });

    int type = lua_type(L, 2);
    if (type == LUA_TTABLE) {
        session->SetPayload(lua_checkpayload(L, 2));
    }
    else if (type == LUA_TSTRING) {
        size_t size;
        const char* str = luaL_checklstring(L, 2, &size);
        session->SetBody(cpr::Body(str, size));
    }
    else {
        luaL_error(L, "Expected either a table or a string, got %s", lua_typename(L, type));
    }

    luaL_checktype(L, 3, LUA_TFUNCTION);

    cpr::Header headers = getDefaultHeader();
    if (lua_gettop(L) == 4) { // [headers]
        headers = lua_checkheaders(L, 4);
    }

    session->SetHeader(headers);

    switch (method) {
    case postRquestEnum::post:
        pushResponse(L, 3, std::move(session->PostAsync()));

        break;
    case postRquestEnum::patch:
        pushResponse(L, 3, std::move(session->PatchAsync()));

        break;
    case postRquestEnum::put:
        pushResponse(L, 3, std::move(session->PutAsync()));

        break;
    default:
        assert();
        return;
    }
}

int get(lua_State* L) {
    genericGetRequest(L, getRquestEnum::get);
    return 0;
}

int del(lua_State* L) {
    genericGetRequest(L, getRquestEnum::del);
    return 0;
}

int head(lua_State* L) {
    genericGetRequest(L, getRquestEnum::head);
    return 0;
}

int options(lua_State* L) {
    genericGetRequest(L, getRquestEnum::options);
    return 0;
}

int post(lua_State* L) {
    genericPostRequest(L, postRquestEnum::post);
    return 0;
}

int put(lua_State* L) {
    genericPostRequest(L, postRquestEnum::put);
    return 0;
}

int patch(lua_State* L) {
    genericPostRequest(L, postRquestEnum::patch);
    return 0;
}

int cleanup(lua_State* L) {
    if (requests) {
        delete requests;
        requests = nullptr;
    }

    return 0;
}

extern "C" {
    __declspec(dllexport) int luaopen_sm_http(lua_State* L) {
        requests = new std::vector<Request>();

        luaL_register(L, "sm_http", functions);

        // for cleaning up our stuff
        lua_newuserdata(L, sizeof(void*));
        luaL_newmetatable(L, "sm_http.cleanup");
        lua_pushcfunction(L, cleanup);
        lua_setfield(L, -2, "__gc");

        lua_setmetatable(L, -2);
        luaL_ref(L, LUA_REGISTRYINDEX);
     
        return 1;
    }
}