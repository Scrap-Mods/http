#pragma once

#include <luajit/lua.hpp>
#include <cpr/cpr.h> // https://docs.libcpr.org/
#include <vector>
#include <cpr/curl_container.h>

struct Request {
    cpr::AsyncWrapper<cpr::Response, false> future_response;
    int callbackRef;
    lua_State* state;
};

std::vector<Request> requests;

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

int tick(lua_State*);
int request(lua_State* L);
int post(lua_State* L);

static const struct luaL_Reg functions[] = {
    {"request", request},
    {"post", post},
    {"tick", tick},
    {NULL, NULL}
};