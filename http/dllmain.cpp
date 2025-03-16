#include <luajit/lua.hpp>
#include <cpr/cpr.h> // https://docs.libcpr.org/
#include <cpr/curl_container.h>

#include <vector>

struct Request
{
	inline Request(cpr::AsyncResponse&& future_response, int callbackref, lua_State* state) :
		m_future_response(std::move(future_response)),
		m_callbackRef(callbackref),
		m_state(state)
	{}

	inline static std::vector<Request> Queue;

	cpr::AsyncResponse m_future_response;
	int m_callbackRef;
	lua_State* m_state;
};

enum class ERequestType : std::uint8_t
{
	// Post reqeusts
	post,
	put,
	patch,

	// Get requests
	get,
	del,
	head,
	options
};

static cpr::Header getDefaultHeader()
{
	return {
		{
			"User-Agent",
			"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36"
		}
	};
}

static void lua_checkargs(lua_State* L, const int argc)
{
	const int args = lua_gettop(L);

	if (args != argc)
	{
		luaL_error(L, "Expected %d argument, got %d", argc, args);
	}
}

static void lua_checkargs(lua_State* L, const int argc, bool optionals)
{
	const int args = lua_gettop(L);

	if (args < argc)
	{
		luaL_error(L, "Expected atleast %d argument, got %d", argc, args);
	}
}

static cpr::Header lua_checkheaders(lua_State* L, int narg)
{
	luaL_checktype(L, narg, LUA_TTABLE);
	cpr::Header header;

	lua_pushnil(L);
	while (lua_next(L, narg) != 0)
	{
		size_t key_size;
		size_t value_size;
		const char* key = luaL_checklstring(L, -2, &key_size);
		const char* value = luaL_checklstring(L, -1, &value_size);

		header[std::string(key, key_size)] = std::string(value, value_size);
		lua_pop(L, 1);
	}

	return header;
}

static cpr::Payload lua_checkpayload(lua_State* L, int narg)
{
	luaL_checktype(L, narg, LUA_TTABLE);
	std::vector<cpr::Pair> pairs;

	lua_pushnil(L);
	while (lua_next(L, narg) != 0)
	{
		size_t key_size;
		size_t value_size;
		const char* key = luaL_checklstring(L, -2, &key_size);
		const char* value = luaL_checklstring(L, -1, &value_size);

		pairs.emplace_back(std::string(key, key_size), std::string(value, value_size));
		lua_pop(L, 1);
	}

	return cpr::Payload(pairs.begin(), pairs.end());
}

static void lua_parseresponse(lua_State* L, const cpr::Response& response)
{
	lua_createtable(L, 0, 8);

	lua_pushlstring(L, response.text.c_str(), response.text.size());
	lua_setfield(L, -2, "body");

	lua_pushlstring(L, response.url.str().c_str(), response.url.str().size());
	lua_setfield(L, -2, "url");

	lua_pushnumber(L, (int)response.status_code);
	lua_setfield(L, -2, "status");

	lua_pushnumber(L, (int)response.error.code);
	lua_setfield(L, -2, "error_code");

	lua_pushnumber(L, (int)response.redirect_count);
	lua_setfield(L, -2, "redirect_count");

	lua_pushlstring(L, response.error.message.c_str(), response.error.message.size());
	lua_setfield(L, -2, "error");

	lua_pushlstring(L, response.reason.c_str(), response.reason.size());
	lua_setfield(L, -2, "reason");

	lua_createtable(L, 0, (int)response.header.size());
	for (const auto& header : response.header)
	{
		lua_pushlstring(L, header.second.c_str(), header.second.size());
		lua_setfield(L, -2, header.first.c_str());
	}
	lua_setfield(L, -2, "headers");
}

static int tick(lua_State*)
{
	auto& v_queue = Request::Queue;

	for (size_t i = v_queue.size(); i-- > 0;)
	{
		auto& request = v_queue[i];
		if (request.m_future_response.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
			continue;

		cpr::Response response = request.m_future_response.get();
		lua_State* L = request.m_state;

		lua_rawgeti(L, LUA_REGISTRYINDEX, request.m_callbackRef);
		lua_parseresponse(L, response);
		lua_call(L, 1, 0);

		luaL_unref(L, LUA_REGISTRYINDEX, request.m_callbackRef);
		v_queue.erase(v_queue.begin() + i); // pop the request
	}

	return 0;
}

inline static void pushResponse(lua_State* L, int narg, cpr::AsyncResponse&& m_future_response)
{
	lua_pushvalue(L, narg);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);

	Request::Queue.emplace_back(std::move(m_future_response), ref, L);
}

static void luaGetRequest(lua_State* L, ERequestType method)
{
	// Do all the checks that might throw exceptions before allocating any memory
	lua_checkargs(L, 2, true);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	size_t size;
	const char* url = luaL_checklstring(L, 1, &size);

	std::shared_ptr<cpr::Session> session = std::make_shared<cpr::Session>();
	session->SetUrl(cpr::Url(url, size));
	session->SetTimeout(cpr::Timeout{ 60 * 1000 });
	session->SetHeader(
		(lua_gettop(L) == 3)
		? lua_checkheaders(L, 3)
		: getDefaultHeader()
	);

	switch (method)
	{
	case ERequestType::get:
		pushResponse(L, 2, std::move(session->GetAsync()));
		break;
	case ERequestType::del:
		pushResponse(L, 2, std::move(session->DeleteAsync()));
		break;
	case ERequestType::head:
		pushResponse(L, 2, std::move(session->HeadAsync()));
		break;
	case ERequestType::options:
		pushResponse(L, 2, std::move(session->OptionsAsync()));
		break;
	default:
		assert("Invalid get request method");
		return;
	}
}

static void luaPostRequest(lua_State* L, ERequestType method)
{
	lua_checkargs(L, 3, true);
	luaL_checktype(L, 3, LUA_TFUNCTION);

	size_t size;
	const char* url = luaL_checklstring(L, 1, &size);

	std::shared_ptr<cpr::Session> session = std::make_shared<cpr::Session>();
	session->SetUrl(cpr::Url(url, size));
	session->SetTimeout(cpr::Timeout{ 60 * 1000 });

	const int payloadType = lua_type(L, 2);
	if (payloadType == LUA_TTABLE)
	{
		session->SetPayload(lua_checkpayload(L, 2));
	}
	else if (payloadType == LUA_TSTRING)
	{
		size_t size;
		const char* str = luaL_checklstring(L, 2, &size);

		session->SetBody(cpr::Body(str, size));
	}
	else
	{
		luaL_error(L, "Expected either a table or a string, got %s", lua_typename(L, payloadType));
		return;
	}

	session->SetHeader(
		(lua_gettop(L) == 4)
		? lua_checkheaders(L, 4)
		: getDefaultHeader()
	);

	switch (method)
	{
	case ERequestType::post:
		pushResponse(L, 3, std::move(session->PostAsync()));
		break;
	case ERequestType::patch:
		pushResponse(L, 3, std::move(session->PatchAsync()));
		break;
	case ERequestType::put:
		pushResponse(L, 3, std::move(session->PutAsync()));
		break;
	default:
		assert("Invalid post request method");
		return;
	}
}

static int cleanup(lua_State* L)
{
	Request::Queue.clear();
	Request::Queue.shrink_to_fit();
	return 0;
}

template<void (TReqFunc)(lua_State*, ERequestType), ERequestType t_req_type>
inline constexpr int luaRequestT(lua_State* L)
{
	TReqFunc(L, t_req_type);
	return 0;
}

static const struct luaL_Reg g_httpFunctions[] =
{
	// GET requests
	{ "get"    , luaRequestT<luaGetRequest, ERequestType::get>     },
	{ "del"    , luaRequestT<luaGetRequest, ERequestType::del>     },
	{ "head"   , luaRequestT<luaGetRequest, ERequestType::head>    },
	{ "options", luaRequestT<luaGetRequest, ERequestType::options> },

	// POST requests
	{ "post" , luaRequestT<luaPostRequest, ERequestType::post>  },
	{ "put"  , luaRequestT<luaPostRequest, ERequestType::put>   },
	{ "patch", luaRequestT<luaPostRequest, ERequestType::patch> },

	{ "tick", tick },
	{ NULL, NULL }
};

extern "C"
{
	__declspec(dllexport) int luaopen_http(lua_State* L)
	{
		luaL_register(L, "http", g_httpFunctions);

		// for cleaning up our stuff
		lua_newuserdata(L, sizeof(void*));
		luaL_newmetatable(L, "http.cleanup");
		lua_pushcfunction(L, cleanup);
		lua_setfield(L, -2, "__gc");

		lua_setmetatable(L, -2);
		luaL_ref(L, LUA_REGISTRYINDEX);
	 
		return 1;
	}
}