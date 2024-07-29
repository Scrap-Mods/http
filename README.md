<p align="center">
  <img src="https://github.com/user-attachments/assets/89664392-1fbe-4b23-a9e1-65b07fa14702"/>
</p>

# Overview 📖

Welcome to the **HTTP Lua Library**, a sleek and non-blocking solution for making HTTP requests in Lua, powered by a robust C++ backend. This library supports various HTTP methods, including `GET`, `POST`, `PUT`, `DELETE`, and more, enabling you to perform asynchronous HTTP operations with ease and efficiency. 🎉

## Installation 📦

Getting started is simple! Just follow these steps:

1. **📥 Download the Latest Release**: Head over to our GitHub repository and grab the latest version.
2. **🔗 Link the Library**: Place the downloaded library file (`http.so`, `http.dll`, etc.) in a directory where your Lua interpreter can find it.
3. **📚 Load the Library**: Use the `require` function in your Lua scripts to bring in the magic.

```lua
local http = require("http")
```

## Usage 💻

### Making Requests 🚀

This library offers a variety of functions for making different types of HTTP requests, all non-blocking and callback-driven.

#### GET Request 🔍

```lua
http.get("http://www.httpbin.org/get", function(response)
    print(response.body)
end)
```

#### DELETE Request 🗑️

```lua
http.del("http://www.httpbin.org/delete", function(response)
    print(response.status)
end)
```

#### HEAD Request 🧾

```lua
http.head("http://www.httpbin.org/get", function(response)
    print(response.headers)
end)
```

#### OPTIONS Request 🎛️

```lua
http.options("http://www.httpbin.org/get", function(response)
    print(response.headers)
end)
```

#### POST Request ✉️

```lua
http.post("http://www.httpbin.org/post", {key = "value"}, function(response)
    print(response.body)
end)
```

#### PUT Request 📝

```lua
http.put("http://www.httpbin.org/put", "raw data", function(response)
    print(response.status)
end)
```

#### PATCH Request 🔧

```lua
http.patch("http://www.httpbin.org/patch", {key = "new value"}, function(response)
    print(response.reason)
end)
```

### Request Parameters 📋

Parameters vary depending on the type of request:

#### GET Requests and Similar 🔍

- **url (string)**: The URL to send the request to.
- **callback (function)**: The function to handle the response.
- **headers (table, optional)**: Additional headers as key-value pairs.

#### POST Requests and Similar ✉️

- **url (string)**: The URL to send the request to.
- **payload (table|string)**: The data sent with the request. This can be:
  - A table of key-value pairs, or
  - A raw body string.
- **callback (function)**: The function to handle the response.
- **headers (table, optional)**: Additional headers as key-value pairs.

### Response Table Structure 🗃️

The response table provided to the callback includes:

- **body (string)**: The response body.
- **url (string)**: The requested URL.
- **status (number)**: The HTTP status code.
- **error_code (number)**: Error code, if any.
- **redirect_count (number)**: Number of redirects followed.
- **error (string)**: Error message, if any.
- **reason (string)**: Status code reason phrase.
- **headers (table)**: Response headers as key-value pairs.

### Example 📘

```lua
local http = require("http")

http.get("http://www.httpbin.org/get", function(response)
    if response.status == 200 then
        print("Response body:", response.body)
    else
        print("Error:", response.error)
    end
end)
```
