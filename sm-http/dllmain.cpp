#include "dllmain.hpp"

/*
    local sm_http = require("sm_http")

    sm_http.request("google.com", function(data, url, status, headers)
        print(data)
    end)
*/

int tick(lua_State*) {
    for (size_t i = requests.size(); i-- > 0;) {
        auto& request = requests[i];
        lua_State* L = request.state;

        if (request.future_response.wait_for(std::chrono::seconds(0)) != std::future_status::ready) continue;
            
        // call callback(data, url)
        cpr::Response response = request.future_response.get();

        lua_rawgeti(L, LUA_REGISTRYINDEX, request.callbackRef);

        lua_pushstring(L, response.text.c_str());
        lua_pushstring(L, response.url.c_str());
        lua_pushnumber(L, response.status_code);

        lua_createtable(L, 0, 5);

        for (const auto& header : response.header) {
            lua_pushstring(L, header.second.c_str());
            lua_setfield(L, -2, header.first.c_str());
        }

        lua_call(L, 4, 0);

        luaL_unref(L, LUA_REGISTRYINDEX, request.callbackRef);

        requests.erase(requests.begin() + i); // pop the request
	}

	return 0;
}

// sm_http.request(url, callback, [headers])
int request(lua_State* L) {
    lua_checkargs(L, 2, true);

    size_t size;
    const char* urltmp = luaL_checklstring(L, 1, &size); // url
    std::string url(urltmp, size);

    luaL_checktype(L, 2, LUA_TFUNCTION); // callback

    cpr::Header headers;

    if (lua_gettop(L) == 3) { // [headers]
        headers = lua_checkheaders(L, 3);
        lua_pushvalue(L, 2);
    }

    auto future_response = cpr::GetAsync(
        cpr::Url{ url },
        cpr::Timeout{5000},
        headers
    );

    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    Request request = { std::move(future_response), ref, L };
    requests.push_back( std::move(request) );

    return 0;
}

// sm_http.post(url, callback, payload)
int post(lua_State* L) {
    lua_checkargs(L, 3);

    size_t size;
    const char* urltmp = luaL_checklstring(L, 1, &size);
    std::string url(urltmp, size);

    luaL_checktype(L, 2, LUA_TFUNCTION);

    auto future_response = cpr::PostAsync(
        cpr::Url{ url },
        cpr::Timeout{ 5000 },
        lua_checkpayload(L, 3)
    );

    lua_pushvalue(L, 2);

    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    Request request = { std::move(future_response), ref, L };
    requests.push_back(std::move(request));

    return 0;
}

extern "C" {
    __declspec(dllexport) int luaopen_sm_http(lua_State* L) {
        luaL_register(L, "sm_http", functions);
        return 1;
    }
}

// TODO: use garbage collection to cleanup when changing state