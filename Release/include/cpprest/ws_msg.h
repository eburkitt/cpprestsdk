/***
* ==++==
*
* Copyright (c) Microsoft Corporation. All rights reserved.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* ws_msg.h
*
* Websocket incoming and outgoing message definitions.
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/
#pragma once

#ifndef _CASA_WS_MSG_H
#define _CASA_WS_MSG_H

#if (defined(__cplusplus_winrt) || !defined(_M_ARM)) && !defined(CPPREST_EXCLUDE_WEBSOCKETS)

#include <memory>
#include <limits>

#include "pplx/pplxtasks.h"
#include "cpprest/details/xxpublic.h"
#include "cpprest/producerconsumerstream.h"
#include "cpprest/uri.h"
#include "cpprest/basic_types.h"
#include "cpprest/asyncrt_utils.h"

namespace web
{
namespace websockets
{
namespace client
{

namespace details
{
    class winrt_client;
    class wspp_client;
#if defined(__cplusplus_winrt)
    ref class ReceiveContext;
#endif
}

/// <summary>
/// The different types of websocket message.
/// Text type contains UTF-8 encoded data.
/// Interpretation of Binary type is left to the application.
/// Note: The fragment types and control frames like close, ping, pong are not supported on WinRT.
/// </summary>
enum class websocket_message_type
{
    text_message,
    binary_message,
    close,
    ping,
    pong
};

/// <summary>
/// Represents an outgoing websocket message
/// </summary>
class websocket_outgoing_message
{
public:

    /// <summary>
    /// Sets a UTF-8 message as the message body.
    /// </summary>
    /// <param name="data">UTF-8 String containing body of the message.</param>
    void set_utf8_message(std::string &&data)
    {
        this->set_message(concurrency::streams::container_buffer<std::string>(std::move(data)));
    }

    /// <summary>
    /// Sets a UTF-8 message as the message body.
    /// </summary>
    /// <param name="data">UTF-8 String containing body of the message.</param>
    void set_utf8_message(const std::string &data)
    {
        this->set_message(concurrency::streams::container_buffer<std::string>(data));
    }

    /// <summary>
    /// Sets a UTF-8 message as the message body.
    /// </summary>
    /// <param name="istream">casablanca input stream representing the body of the message.</param>
    /// <remarks>Upon sending, the entire stream may be buffered to determine the length.</remarks>
    void set_utf8_message(const concurrency::streams::istream &istream)
    {
        this->set_message(istream, SIZE_MAX, websocket_message_type::text_message);
    }

    /// <summary>
    /// Sets a UTF-8 message as the message body.
    /// </summary>
    /// <param name="istream">casablanca input stream representing the body of the message.</param>
    /// <param name="len">number of bytes to send.</param>
    void set_utf8_message(const concurrency::streams::istream &istream, size_t len)
    {
        this->set_message(istream, len, websocket_message_type::text_message);
    }

    /// <summary>
    /// Sets binary data as the message body.
    /// </summary>
    /// <param name="istream">casablanca input stream representing the body of the message.</param>
    /// <param name="len">number of bytes to send.</param>
    void set_binary_message(const concurrency::streams::istream &istream, size_t len)
    {
        this->set_message(istream, len, websocket_message_type::binary_message);
    }

    /// <summary>
    /// Sets binary data as the message body.
    /// </summary>
    /// <param name="istream">Input stream representing the body of the message.</param>
    /// <remarks>Upon sending, the entire stream may be buffered to determine the length.</remarks>
    void set_binary_message(const concurrency::streams::istream &istream)
    {
        this->set_message(istream, SIZE_MAX, websocket_message_type::binary_message);
    }

private:
    friend class details::winrt_client;
    friend class details::wspp_client;

    pplx::task_completion_event<void> m_body_sent;
    concurrency::streams::streambuf<uint8_t> m_body;
    websocket_message_type m_msg_type;
    size_t m_length;

    void signal_body_sent() const
    {
        m_body_sent.set();
    }

    void signal_body_sent(const std::exception_ptr &e) const
    {
        m_body_sent.set_exception(e);
    }

    const pplx::task_completion_event<void> & body_sent() const { return m_body_sent; }

    void set_message(const concurrency::streams::container_buffer<std::string> &buffer)
    {
        m_msg_type = websocket_message_type::text_message;
        m_length = static_cast<size_t>(buffer.size());
        m_body = buffer;
    }

    void set_message(const concurrency::streams::istream &istream, size_t len, websocket_message_type msg_type)
    {
        m_msg_type = msg_type;
        m_length = len;
        m_body = istream.streambuf();
    }
};

/// <summary>
/// Represents an incoming websocket message
/// </summary>
class websocket_incoming_message
{
public:

    /// <summary>
    /// Extracts the body of the incoming message as a string value, only if the message type is UTF-8.
    /// A body can only be extracted once because in some cases an optimization is made where the data is 'moved' out.
    /// </summary>
    /// <returns>String containing body of the message.</returns>
    _ASYNCRTIMP pplx::task<std::string> extract_string() const;

    /// <summary>
    /// Produces a stream which the caller may use to retrieve body from an incoming message.
    /// Can be used for both UTF-8 (text) and binary message types.
    /// </summary>
    /// <returns>A readable, open asynchronous stream.</returns>
    /// <remarks>
    /// This cannot be used in conjunction with any other means of getting the body of the message.
    /// </remarks>
    concurrency::streams::istream body() const
    {
        auto to_uint8_t_stream = [](const concurrency::streams::streambuf<uint8_t> &buf) -> concurrency::streams::istream
        {
            return buf.create_istream();
        };
        return to_uint8_t_stream(m_body);
    }

    /// <summary>
    /// Returns the length of the received message.
    /// </summary>
    size_t length() const
    {
        return static_cast<size_t>(m_body.size());
    }

    /// <summary>
    /// Returns the type of the received message.
    /// </summary>
    CASABLANCA_DEPRECATED("Incorrectly spelled API, use message_type() instead.")
    websocket_message_type messge_type() const
    {
        return m_msg_type;
    }

    /// <summary>
    /// Returns the type of the received message, either string or binary.
    /// </summary>
    /// <returns>websocket_message_type</returns>
    websocket_message_type message_type() const
    {
        return m_msg_type;
    }

private:
    friend class details::winrt_client;
    friend class details::wspp_client;
#if defined(__cplusplus_winrt)
    friend ref class details::ReceiveContext;
#endif

    // Store message body in a container buffer backed by a string.
    // Allows for optimization in the string message cases.
    concurrency::streams::container_buffer<std::string> m_body;
    websocket_message_type m_msg_type;
};

}}}

#endif
#endif  /* _CASA_WS_MSG_H */
