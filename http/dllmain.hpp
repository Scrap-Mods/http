#pragma once

#include <luajit/lua.hpp>
#include <cpr/cpr.h> // https://docs.libcpr.org/
#include <vector>
#include <cpr/curl_container.h>

inline cpr::Header getDefaultHeader() {
    return {
        {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36"}
    };
}

struct Request {
    cpr::AsyncResponse m_future_response;
    int m_callbackRef;
    lua_State* m_state;

    Request(cpr::AsyncResponse&& future_response, int callbackref, lua_State* state) :
        m_future_response(std::move(future_response)),
        m_callbackRef(callbackref),
        m_state(state)
    {}
};

static std::vector<Request>* requests = nullptr;

void lua_checkargs(lua_State* L, const int argc) {
    const int args = lua_gettop(L);

    if (args != argc)
    {
        luaL_error(L, "Expected %d argument, got %d", argc, args);
    }
}

void lua_checkargs(lua_State* L, const int argc, bool optionals) {
    const int args = lua_gettop(L);

    if (args < argc)
    {
        luaL_error(L, "Expected atleast %d argument, got %d", argc, args);
    }
}

cpr::Header lua_checkheaders(lua_State* L, int narg) {
    luaL_checktype(L, narg, LUA_TTABLE);
    cpr::Header header;

    lua_pushnil(L);
    while (lua_next(L, narg) != 0) {
        size_t key_size;
        size_t value_size;
        const char* key = luaL_checklstring(L, -2, &key_size);
        const char* value = luaL_checklstring(L, -1, &value_size);

        header[std::string(key, key_size)] = std::string(value, value_size);
        lua_pop(L, 1);
    }

    return header;
}

cpr::Payload lua_checkpayload(lua_State* L, int narg) {
    luaL_checktype(L, narg, LUA_TTABLE);
    std::vector<cpr::Pair> pairs;

    lua_pushnil(L);
    while (lua_next(L, narg) != 0) {
        size_t key_size;
        size_t value_size;
        const char* key = luaL_checklstring(L, -2, &key_size);
        const char* value = luaL_checklstring(L, -1, &value_size);

        pairs.emplace_back(std::string(key, key_size), std::string(value, value_size));
        lua_pop(L, 1);
    }

    return cpr::Payload(pairs.begin(), pairs.end());
}

enum class postRquestEnum {
    post,
    put,
    patch
};

enum class getRquestEnum {
    get,
    del,
    head,
    options
};

int tick(lua_State*);

int get(lua_State* L);
int del(lua_State* L);
int head(lua_State* L);
int options(lua_State* L);

int post(lua_State* L);
int put(lua_State* L);
int patch(lua_State* L);

static const struct luaL_Reg functions[] = {
    {"get", get},
    {"del", del},
    {"head", head},
    {"options", options},

    {"post", post},
    {"put", put},
    {"patch", patch},

    {"tick", tick},
    {NULL, NULL}
};