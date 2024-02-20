
/**
 *
 * The Network Upgradable BearSSL Client Class, ESP8266_SSL_Client.cpp v2.0.2
 *
 * Created March 1, 2023
 *
 * This works based on Earle F. Philhower ServerSecure class
 *
 * The MIT License (MIT)
 * Copyright (c) 2023 K. Suwatchai (Mobizt)
 *
 *
 * Permission is hereby granted, free of charge, to any person returning a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
  WiFiClientBearSSL- SSL client/server for esp8266 using BearSSL libraries
  - Mostly compatible with Arduino WiFi shield library and standard
    WiFiClient/ServerSecure (except for certificate handling).

  Copyright (c) 2018 Earle F. Philhower, III

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef ESP8266_SSL_Client_CPP
#define ESP8266_SSL_Client_CPP

#include <Arduino.h>
#include "ESP_Mail_FS.h"
#if defined(ESP8266) || defined(MB_ARDUINO_PICO)

#include "ESP8266_SSL_Client.h"
#include "ESP_Mail_Print.h"

#if defined(DEBUG_ESP_SSL) && defined(DEBUG_ESP_PORT)
#define DEBUG_BSSL(fmt, ...) DEBUG_ESP_PORT.printf_P((PGM_P)PSTR("BSSL:" fmt), ##__VA_ARGS__)
#else
#define DEBUG_BSSL(...)
#endif

#include <list>
#include <errno.h>
#include <algorithm>
#include "StackThunk.h"
#if defined(MB_ARDUINO_PICO)
#include "lwip/opt.h"
#include "lwip/ip.h"
#include "lwip/tcp.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#endif

#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE) && !defined(USING_AXTLS)

// #include "c_types.h"
// #include <mmu_iram.h>
// #include <umm_malloc/umm_malloc.h>
// #include <umm_malloc/umm_heap_select.h>
#if 1
#if !CORE_MOCK

// The BearSSL thunks in use for now
#define br_ssl_engine_recvapp_ack thunk_br_ssl_engine_recvapp_ack
#define br_ssl_engine_recvapp_buf thunk_br_ssl_engine_recvapp_buf
#define br_ssl_engine_recvrec_ack thunk_br_ssl_engine_recvrec_ack
#define br_ssl_engine_recvrec_buf thunk_br_ssl_engine_recvrec_buf
#define br_ssl_engine_sendapp_ack thunk_br_ssl_engine_sendapp_ack
#define br_ssl_engine_sendapp_buf thunk_br_ssl_engine_sendapp_buf
#define br_ssl_engine_sendrec_ack thunk_br_ssl_engine_sendrec_ack
#define br_ssl_engine_sendrec_buf thunk_br_ssl_engine_sendrec_buf

#endif
#endif

namespace BearSSL
{

    void ESP8266_SSL_Client::_clear()
    {
        // TLS handshake may take more than the 5 second default timeout
        _timeout = 15000;

        _sc = nullptr;
        _sc_svr = nullptr;
        _eng = nullptr;
        _x509_minimal = nullptr;
        _x509_insecure = nullptr;
        _x509_knownkey = nullptr;
        _iobuf_in = nullptr;
        _iobuf_out = nullptr;
        _now = 0; // You can override or ensure time() is correct w/configTime
        _ta = nullptr;
        setBufferSizes(16384, 512); // Minimum safe
        _handshake_done = false;
        _recvapp_buf = nullptr;
        _recvapp_len = 0;
        _oom_err = false;
        _session = nullptr;
        _cipher_list = nullptr;
        _cipher_cnt = 0;
        _tls_min = BR_TLS10;
        _tls_max = BR_TLS12;
        if (_esp32_ta)
        {
            delete _esp32_ta;
            _esp32_ta = nullptr;
        }
    }

    void ESP8266_SSL_Client::_clearAuthenticationSettings()
    {
        _use_insecure = false;
        _use_fingerprint = false;
        _use_self_signed = false;
        _knownkey = nullptr;
        _ta = nullptr;
        if (_esp32_ta)
        {
            delete _esp32_ta;
            _esp32_ta = nullptr;
        }
    }

    ESP8266_SSL_Client::ESP8266_SSL_Client()
    {
        _clear();
        _clearAuthenticationSettings();
        _certStore = nullptr; // Don't want to remove cert store on a clear, should be long lived
        _sk = nullptr;
        stack_thunk_add_ref();
    }

    ESP8266_SSL_Client::~ESP8266_SSL_Client()
    {
        if (base_client)
        {
            if (base_client->connected())
                base_client->stop();
            base_client = nullptr;
        }
        _cipher_list = nullptr; // std::shared will free if last reference
        _freeSSL();
        stack_thunk_del_ref();
    }

    void ESP8266_SSL_Client::setClientRSACert(const X509List *chain, const PrivateKey *sk)
    {
        if (_esp32_chain)
        {
            delete _esp32_chain;
            _esp32_chain = nullptr;
        }
        if (_esp32_sk)
        {
            delete _esp32_sk;
            _esp32_sk = nullptr;
        }
        _chain = chain;
        _sk = sk;
    }

    void ESP8266_SSL_Client::setClientECCert(const X509List *chain,
                                             const PrivateKey *sk, unsigned allowed_usages, unsigned cert_issuer_key_type)
    {
        if (_esp32_chain)
        {
            delete _esp32_chain;
            _esp32_chain = nullptr;
        }
        if (_esp32_sk)
        {
            delete _esp32_sk;
            _esp32_sk = nullptr;
        }
        _chain = chain;
        _sk = sk;
        _allowed_usages = allowed_usages;
        _cert_issuer_key_type = cert_issuer_key_type;
    }

    void ESP8266_SSL_Client::setBufferSizes(int recv, int xmit)
    {
        // Following constants taken from bearssl/src/ssl/ssl_engine.c (not exported unfortunately)
        const int MAX_OUT_OVERHEAD = 85;
        const int MAX_IN_OVERHEAD = 325;

        // The data buffers must be between 512B and 16KB
        recv = std::max(512, std::min(16384, recv));
        xmit = std::max(512, std::min(16384, xmit));

        // Add in overhead for SSL protocol
        recv += MAX_IN_OVERHEAD;
        xmit += MAX_OUT_OVERHEAD;
        _iobuf_in_size = recv;
        _iobuf_out_size = xmit;
    }

    void ESP8266_SSL_Client::stop()
    {
        if (base_client)
            base_client->stop();

        if (!secured)
            return;

        // Only if we've already connected, store session params and clear the connection options
        if (_handshake_done)
        {
            if (_session)
            {
                br_ssl_engine_get_session_parameters(_eng, _session->getSession());
            }
        }
        _freeSSL();
    }

    void ESP8266_SSL_Client::flush()
    {
        if (!base_client)
            return;

        if (secured)
            (void)_run_until(BR_SSL_SENDAPP);

        base_client->flush();
    }

    int ESP8266_SSL_Client::connect(IPAddress ip, uint16_t port)
    {
        if (!base_client)
        {
            if (debugLevel > 0)
                ESP_MAIL_PRINTF((const char *)FPSTR("> E: Client is not yet initialized\n"));
            return 0;
        }

#if defined(ENABLE_CUSTOM_CLIENT)

        if (connection_cb)
            connection_cb(ip.toString().c_str(), port);
        else
            base_client->connect(ip, port);

        if (!connected())
            return 0;

#else

        if (!base_client->connect(ip, port))
        {
            DEBUG_BSSL("connect: base client connection failed\n");
            return 0;
        }

#endif

        host = "";
        return 1;
    }

    int ESP8266_SSL_Client::connect(const char *name, uint16_t port)
    {
        if (!base_client)
        {
            if (debugLevel > 0)
                ESP_MAIL_PRINTF((const char *)FPSTR("> E: Client is not yet initialized\n"));
            return 0;
        }

#if defined(ENABLE_CUSTOM_CLIENT)

        if (connection_cb)
            connection_cb(name, port);
        else
            base_client->connect(name, port);

        if (!connected())
            return 0;

#else

        if (!base_client->connect(name, port))
        {
            DEBUG_BSSL("connect: base client connection failed\n");
            return 0;
        }

#endif
        host = name;
        return 1;
    }

    int ESP8266_SSL_Client::connect(const String &host, uint16_t port)
    {
        return connect(host.c_str(), port);
    }

    void ESP8266_SSL_Client::_freeSSL()
    {
        // These are smart pointers and will free if refcnt==0
        _sc = nullptr;
        _sc_svr = nullptr;
        _x509_minimal = nullptr;
        _x509_insecure = nullptr;
        _x509_knownkey = nullptr;
        _iobuf_in = nullptr;
        _iobuf_out = nullptr;
        // Reset non-allocated ptrs (pointing to bits potentially free'd above)
        _recvapp_buf = nullptr;
        _recvapp_len = 0;
        // This connection is toast
        _handshake_done = false;
        _timeout = 15000;
        secured = false;
    }

    bool ESP8266_SSL_Client::_clientConnected()
    {
        return (base_client && base_client->connected());
    }

    bool ESP8266_SSL_Client::connected()
    {
        if (!base_client)
            return 0;

        if (!secured)
            return base_client->connected();

        if (available() || (_clientConnected() && _handshake_done && (br_ssl_engine_current_state(_eng) != BR_SSL_CLOSED)))
        {
            return 1;
        }
        return 0;
    }

    int ESP8266_SSL_Client::availableForWrite()
    {
        if (!base_client || !secured)
            return 0;

        // code taken from ::_write()
        if (!connected() || !_handshake_done)
        {
            return 0;
        }
        // Get BearSSL to a state where we can send
        if (_run_until(BR_SSL_SENDAPP) < 0)
        {
            return 0;
        }
        if (br_ssl_engine_current_state(_eng) & BR_SSL_SENDAPP)
        {
            size_t sendapp_len;
            (void)br_ssl_engine_sendapp_buf(_eng, &sendapp_len);
            // We want to call br_ssl_engine_sendapp_ack(0) but 0 is forbidden (bssl doc).
            // After checking br_ssl_engine_sendapp_buf() src code,
            // it seems that it is OK to not call ack when the buffer is left untouched.
            // forbidden: br_ssl_engine_sendapp_ack(_eng, 0);
            return (int)sendapp_len;
        }
        return 0;
    }

    size_t ESP8266_SSL_Client::_write(const uint8_t *buf, size_t size, bool pmem)
    {
        if (!base_client)
            return 0;

        if (!secured)
        {
            if (pmem)
            {
                char dest[size];
                memcpy_P((void *)dest, (PGM_VOID_P)buf, size);
                return base_client->write((uint8_t *)dest, size);
            }

            return base_client->write(buf, size);
        }

        size_t sent_bytes = 0;

        if (!connected() || !size || !_handshake_done)
        {
            return 0;
        }

        do
        {
// Ensure we yield if we need multiple fragments to avoid WDT
#if defined(ESP8266)
            if (sent_bytes)
            {
                optimistic_yield(1000);
            }
#endif
            // Get BearSSL to a state where we can send
            if (_run_until(BR_SSL_SENDAPP) < 0)
            {
                break;
            }

            if (br_ssl_engine_current_state(_eng) & BR_SSL_SENDAPP)
            {
                size_t sendapp_len;
                unsigned char *sendapp_buf = br_ssl_engine_sendapp_buf(_eng, &sendapp_len);
                int to_send = size > sendapp_len ? sendapp_len : size;
                if (pmem)
                {
                    memcpy_P(sendapp_buf, buf, to_send);
                }
                else
                {
                    memcpy(sendapp_buf, buf, to_send);
                }
                br_ssl_engine_sendapp_ack(_eng, to_send);
                br_ssl_engine_flush(_eng, 0);
                flush();
                buf += to_send;
                sent_bytes += to_send;
                size -= to_send;
            }
            else
            {
                break;
            }
        } while (size);

        return sent_bytes;
    }

    size_t ESP8266_SSL_Client::write(const uint8_t *buf, size_t size)
    {
        return _write(buf, size, false);
    }

    size_t ESP8266_SSL_Client::write_P(PGM_P buf, size_t size)
    {
        return _write((const uint8_t *)buf, size, true);
    }

    size_t ESP8266_SSL_Client::write(Stream &stream)
    {
        if (!connected() || !_handshake_done)
        {
            DEBUG_BSSL("write: Connect/handshake not completed yet\n");
            return 0;
        }

        size_t dl = stream.available();
        uint8_t buf[dl];
        stream.readBytes(buf, dl);
        return _write(buf, dl, false);
    }

    int ESP8266_SSL_Client::read(uint8_t *buf, size_t size)
    {
        if (!base_client)
            return 0;

        if (!secured)
            return base_client->read(buf, size);

        if (!ctx_present() || !_handshake_done)
        {
            return -1;
        }

        int avail = available();
        bool conn = connected();
        if (!avail && conn)
        {
            return 0; // We're still connected, but nothing to read
        }
        if (!avail && !conn)
        {
            DEBUG_BSSL("read: Not connected, none left available\n");
            return -1;
        }

        if (avail)
        {
            // Take data from the recvapp buffer
            int to_copy = _recvapp_len < size ? _recvapp_len : size;
            memcpy(buf, _recvapp_buf, to_copy);
            br_ssl_engine_recvapp_ack(_eng, to_copy);
            _recvapp_buf = nullptr;
            _recvapp_len = 0;
            return to_copy;
        }

        if (!conn)
        {
            DEBUG_BSSL("read: Not connected\n");
            return -1;
        }
        return 0; // If we're connected, no error but no read.
    }

#if defined(ESP8266)
    // return a pointer to available data buffer (size = peekAvailable())
    // semantic forbids any kind of read() before calling peekConsume()
    const char *ESP8266_SSL_Client::peekBuffer()
    {
        return (const char *)_recvapp_buf;
    }

    // consume bytes after use (see peekBuffer)
    void ESP8266_SSL_Client::peekConsume(size_t consume)
    {
        // according to ESP8266_SSL_Client::read:
        br_ssl_engine_recvapp_ack(_eng, consume);
        _recvapp_buf = nullptr;
        _recvapp_len = 0;
    }
#endif

    int ESP8266_SSL_Client::read()
    {
        if (!base_client)
            return -1;

        if (!secured)
            return base_client->read();

        uint8_t c;
        if (1 == read(&c, 1))
        {
            return c;
        }
        DEBUG_BSSL("read: failed\n");
        return -1;
    }

    int ESP8266_SSL_Client::available()
    {
        if (!base_client)
            return 0;

        if (!secured)
            return base_client->available();

        if (_recvapp_buf)
        {
            return _recvapp_len; // Anything from last call?
        }
        _recvapp_buf = nullptr;
        _recvapp_len = 0;
        if (!ctx_present() || _run_until(BR_SSL_RECVAPP, false) < 0)
        {
            return 0;
        }

        int st = br_ssl_engine_current_state(_eng);
        if (st == BR_SSL_CLOSED)
        {
            return 0; // Nothing leftover, SSL is closed
        }
        if (st & BR_SSL_RECVAPP)
        {
            _recvapp_buf = br_ssl_engine_recvapp_buf(_eng, &_recvapp_len);
            return _recvapp_len;
        }

        return 0;
    }

    int ESP8266_SSL_Client::peek()
    {
        if (!ctx_present() || !available())
        {
            DEBUG_BSSL("peek: Not connected, none left available\n");
            return -1;
        }

        if (!secured)
            return base_client->peek();

        if (_recvapp_buf && _recvapp_len)
        {
            return _recvapp_buf[0];
        }
        DEBUG_BSSL("peek: No data left\n");
        return -1;
    }

    size_t ESP8266_SSL_Client::peekBytes(uint8_t *buffer, size_t length)
    {
        if (!base_client || !secured)
            return 0;

        size_t to_copy = 0;
        if (!ctx_present())
        {
            DEBUG_BSSL("peekBytes: Not connected\n");
            return 0;
        }

        unsigned long _startMillis = millis();
        while ((available() < (int)length) && ((millis() - _startMillis) < 5000))
        {
            yield();
        }

        to_copy = _recvapp_len < length ? _recvapp_len : length;
        memcpy(buffer, _recvapp_buf, to_copy);
        return to_copy;
    }

    /* --- Copied almost verbatim from BEARSSL SSL_IO.C ---
       Run the engine, until the specified target state is achieved, or
       an error occurs. The target state is SENDAPP, RECVAPP, or the
       combination of both (the combination matches either). When a match is
       achieved, this function returns 0. On error, it returns -1.
    */

    int ESP8266_SSL_Client::_run_until(unsigned int target, bool blocking)
    {
        if (!ctx_present())
        {
            DEBUG_BSSL("_run_until: Not connected\n");
            return -1;
        }

#if defined(ESP8266)
        esp8266::polledTimeout::oneShotMs loopTimeout(_timeout);
#else
        uint32_t start = millis();
#endif

        for (int no_work = 0; blocking || no_work < 2;)
        {
            idle();

#if defined(ESP8266)
            optimistic_yield(100);
            if (loopTimeout)
            {
                DEBUG_BSSL("_run_until: Timeout\n");
                return -1;
            }
#else
            if (millis() - start > _timeout)
            {
                DEBUG_BSSL("_run_until: Timeout\n");
                return -1;
            }
#endif

            int state;
            state = br_ssl_engine_current_state(_eng);
            if (state & BR_SSL_CLOSED)
            {
                DEBUG_BSSL("_run_until: BSSL closed\n");
                return -1;
            }

            if (!base_client->connected() && !base_client->available())
            {
                // Don't print any debug here because of iteration;
                return (state & target) ? 0 : -1;
            }

            /*
               If there is some record data to send, do it. This takes
               precedence over everything else.
            */
            if (state & BR_SSL_SENDREC)
            {
                unsigned char *buf;
                size_t len;
                int wlen;
                size_t availForWrite;

                buf = br_ssl_engine_sendrec_buf(_eng, &len);

#if defined(ESP8266)
#if defined(ESP8266_CORE_SDK_V3_X_X)
                availForWrite = base_client->availableForWrite();

                if (!blocking && len > availForWrite)
                {
                    /*
                       writes on WiFiClient will block if len > availableForWrite()
                       this is needed to prevent available() calls from blocking
                       on dropped connections
                    */
                    len = availForWrite;
                }
#endif
#else
                availForWrite = base_client->availableForWrite();

                if (!blocking && len > availForWrite)
                {
                    /*
                        writes on WiFiClient will block if len > availableForWrite()
                        this is needed to prevent available() calls from blocking
                        on dropped connections
                    */
                    len = availForWrite;
                }
#endif
                wlen = base_client->write(buf, len);

                if (wlen <= 0)
                {
                    /*
                       If we received a close_notify and we
                       still send something, then we have our
                       own response close_notify to send, and
                       the peer is allowed by RFC 5246 not to
                       wait for it.
                    */
                    return -1;
                }

                if (wlen > 0)
                {
                    br_ssl_engine_sendrec_ack(_eng, wlen);
                }

                no_work = 0;
                continue;
            }

            /*
               If we reached our target, then we are finished.
            */
            if (state & target)
            {
                return 0;
            }

            /*
               If some application data must be read, and we did not
               exit, then this means that we are trying to write data,
               and that's not possible until the application data is
               read. This may happen if using a shared in/out buffer,
               and the underlying protocol is not strictly half-duplex.
               This is unrecoverable here, so we report an error.
            */
            if (state & BR_SSL_RECVAPP)
            {
                DEBUG_BSSL("_run_until: Fatal protocol state\n");
                return -1;
            }

            /*
               If we reached that point, then either we are trying
               to read data and there is some, or the engine is stuck
               until a new record is obtained.
            */
            if (state & BR_SSL_RECVREC)
            {
                if (base_client->available())
                {
                    unsigned char *buf;
                    size_t len;
                    int rlen;

                    buf = br_ssl_engine_recvrec_buf(_eng, &len);

                    int read = 0;
                    int toRead = len;
                    while (toRead > 0 && base_client->available())
                    {
                        rlen = base_client->read(buf + read, toRead);
                        read += rlen;
                        toRead = len - read;
                        idle();
#if defined(ESP8266)
                        if (loopTimeout)
                        {
                            DEBUG_BSSL("_run_until: Timeout\n");
                            return -1;
                        }
#else
                        if (millis() - start > _timeout)
                        {
                            DEBUG_BSSL("_run_until: Timeout\n");
                            return -1;
                        }
#endif
                    }

                    rlen = read;

                    if (rlen < 0)
                    {
                        return -1;
                    }

                    if (rlen > 0)
                    {
                        br_ssl_engine_recvrec_ack(_eng, rlen);
                    }

                    no_work = 0;
                    continue;
                }
            }

            /*
               We can reach that point if the target RECVAPP, and
               the state contains SENDAPP only. This may happen with
               a shared in/out buffer. In that case, we must flush
               the buffered data to "make room" for a new incoming
               record.
            */
            br_ssl_engine_flush(_eng, 0);

            no_work++; // We didn't actually advance here
        }
        // We only get here if we ran through the loop without getting anything done
        return -1;
    }

    bool ESP8266_SSL_Client::_wait_for_handshake()
    {
        _handshake_done = false;
        // Change waiting time out to ensure the handshake done
        unsigned long to = _timeout;
        _timeout = 15000;
        while (!_handshake_done && _clientConnected())
        {
            idle();
            int ret = _run_until(BR_SSL_SENDAPP);
            if (ret < 0)
            {
                DEBUG_BSSL("_wait_for_handshake: failed\n");
                break;
            }
            if (br_ssl_engine_current_state(_eng) & BR_SSL_SENDAPP)
            {
                _handshake_done = true;
            }
#if defined(ESP8266)
            optimistic_yield(1000);
#endif
        }
        // Restore waiting time out
        _timeout = to;
        return _handshake_done;
    }

    static uint8_t htoi(unsigned char c)
    {
        if (c >= '0' && c <= '9')
        {
            return c - '0';
        }
        else if (c >= 'A' && c <= 'F')
        {
            return 10 + c - 'A';
        }
        else if (c >= 'a' && c <= 'f')
        {
            return 10 + c - 'a';
        }
        else
        {
            return 255;
        }
    }

    // Set a fingerprint by parsing an ASCII string
    bool ESP8266_SSL_Client::setFingerprint(const char *fpStr)
    {
        int idx = 0;
        uint8_t c, d;
        uint8_t fp[20];

        while (idx < 20)
        {
            c = pgm_read_byte(fpStr++);
            if (!c)
            {
                break; // String ended, done processing
            }
            d = pgm_read_byte(fpStr++);
            if (!d)
            {
                DEBUG_BSSL("setFingerprint: FP too short\n");
                return false; // Only half of the last hex digit, error
            }
            c = htoi(c);
            d = htoi(d);
            if ((c > 15) || (d > 15))
            {
                DEBUG_BSSL("setFingerprint: Invalid char\n");
                return false; // Error in one of the hex characters
            }
            fp[idx++] = (c << 4) | d;

            // Skip 0 or more spaces or colons
            while (pgm_read_byte(fpStr) && (pgm_read_byte(fpStr) == ' ' || pgm_read_byte(fpStr) == ':'))
            {
                fpStr++;
            }
        }
        if ((idx != 20) || pgm_read_byte(fpStr))
        {
            DEBUG_BSSL("setFingerprint: Garbage at end of fp\n");
            return false; // Garbage at EOL or we didn't have enough hex digits
        }
        return setFingerprint(fp);
    }

    extern "C"
    {

        // BearSSL doesn't define a true insecure decoder, so we make one ourselves
        // from the simple parser.  It generates the issuer and subject hashes and
        // the SHA1 fingerprint, only one (or none!) of which will be used to
        // "verify" the certificate.

        // Private x509 decoder state
        struct br_x509_insecure_context
        {
            const br_x509_class *vtable;
            bool done_cert;
            const uint8_t *match_fingerprint;
            br_sha1_context sha1_cert;
            bool allow_self_signed;
            br_sha256_context sha256_subject;
            br_sha256_context sha256_issuer;
            br_x509_decoder_context ctx;
        };

        // Callback for the x509_minimal subject DN
        static void insecure_subject_dn_append(void *ctx, const void *buf, size_t len)
        {
            br_x509_insecure_context *xc = (br_x509_insecure_context *)ctx;
            br_sha256_update(&xc->sha256_subject, buf, len);
        }

        // Callback for the x509_minimal issuer DN
        static void insecure_issuer_dn_append(void *ctx, const void *buf, size_t len)
        {
            br_x509_insecure_context *xc = (br_x509_insecure_context *)ctx;
            br_sha256_update(&xc->sha256_issuer, buf, len);
        }

        // Callback on the first byte of any certificate
        static void insecure_start_chain(const br_x509_class **ctx, const char *server_name)
        {
            br_x509_insecure_context *xc = (br_x509_insecure_context *)ctx;
            br_x509_decoder_init(&xc->ctx, insecure_subject_dn_append, xc, insecure_issuer_dn_append, xc);
            xc->done_cert = false;
            br_sha1_init(&xc->sha1_cert);
            br_sha256_init(&xc->sha256_subject);
            br_sha256_init(&xc->sha256_issuer);
            (void)server_name;
        }

        // Callback for each certificate present in the chain (but only operates
        // on the first one by design).
        static void insecure_start_cert(const br_x509_class **ctx, uint32_t length)
        {
            (void)ctx;
            (void)length;
        }

        // Callback for each byte stream in the chain.  Only process first cert.
        static void insecure_append(const br_x509_class **ctx, const unsigned char *buf, size_t len)
        {
            br_x509_insecure_context *xc = (br_x509_insecure_context *)ctx;
            // Don't process anything but the first certificate in the chain
            if (!xc->done_cert)
            {
                br_sha1_update(&xc->sha1_cert, buf, len);
                br_x509_decoder_push(&xc->ctx, (const void *)buf, len);
#if defined(DEBUG_ESP_SSL) && defined(DEBUG_ESP_PORT)
                DEBUG_BSSL("CERT: ");
                for (size_t i = 0; i < len; i++)
                {
                    DEBUG_ESP_PORT.printf_P(PSTR("%02x "), buf[i] & 0xff);
                }
                DEBUG_ESP_PORT.printf_P(PSTR("\n"));
#endif
            }
        }

        // Callback on individual cert end.
        static void insecure_end_cert(const br_x509_class **ctx)
        {
            br_x509_insecure_context *xc = (br_x509_insecure_context *)ctx;
            xc->done_cert = true;
        }

        // Callback when complete chain has been parsed.
        // Return 0 on validation success, !0 on validation error
        static unsigned insecure_end_chain(const br_x509_class **ctx)
        {
            const br_x509_insecure_context *xc = (const br_x509_insecure_context *)ctx;
            if (!xc->done_cert)
            {
                DEBUG_BSSL("insecure_end_chain: No cert seen\n");
                return 1; // error
            }

            // Handle SHA1 fingerprint matching
            char res[20];
            br_sha1_out(&xc->sha1_cert, res);
            if (xc->match_fingerprint && memcmp(res, xc->match_fingerprint, sizeof(res)))
            {
#ifdef DEBUG_ESP_SSL
                DEBUG_BSSL("insecure_end_chain: Received cert FP doesn't match\n");
                char buff[3 * sizeof(res) + 1]; // 3 chars per byte XX_, and null
                buff[0] = 0;
                for (size_t i = 0; i < sizeof(res); i++)
                {
                    char hex[4]; // XX_\0
                    snprintf(hex, sizeof(hex), "%02x ", xc->match_fingerprint[i] & 0xff);
                    // strlcat(buff, hex, sizeof(buff));
                }
                DEBUG_BSSL("insecure_end_chain: expected %s\n", buff);
                buff[0] = 0;
                for (size_t i = 0; i < sizeof(res); i++)
                {
                    char hex[4]; // XX_\0
                    snprintf(hex, sizeof(hex), "%02x ", res[i] & 0xff);
                    // strlcat(buff, hex, sizeof(buff));
                }
                DEBUG_BSSL("insecure_end_chain: received %s\n", buff);
#endif
                return BR_ERR_X509_NOT_TRUSTED;
            }

            // Handle self-signer certificate acceptance
            char res_issuer[32];
            char res_subject[32];
            br_sha256_out(&xc->sha256_issuer, res_issuer);
            br_sha256_out(&xc->sha256_subject, res_subject);
            if (xc->allow_self_signed && memcmp(res_subject, res_issuer, sizeof(res_issuer)))
            {
                DEBUG_BSSL("insecure_end_chain: Didn't get self-signed cert\n");
                return BR_ERR_X509_NOT_TRUSTED;
            }

            // Default (no validation at all) or no errors in prior checks = success.
            return 0;
        }

        // Return the public key from the validator (set by x509_minimal)
        static const br_x509_pkey *insecure_get_pkey(const br_x509_class *const *ctx, unsigned *usages)
        {
            const br_x509_insecure_context *xc = (const br_x509_insecure_context *)ctx;
            if (usages != nullptr)
            {
                *usages = BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN; // I said we were insecure!
            }
            return &xc->ctx.pkey;
        }

        //  Set up the x509 insecure data structures for BearSSL core to use.
        void _mb_bssl_br_x509_insecure_init(br_x509_insecure_context *ctx, int _use_fingerprint, const uint8_t _fingerprint[20], int _allow_self_signed)
        {
            static const br_x509_class br_x509_insecure_vtable PROGMEM = {
                sizeof(br_x509_insecure_context),
                insecure_start_chain,
                insecure_start_cert,
                insecure_append,
                insecure_end_cert,
                insecure_end_chain,
                insecure_get_pkey};

            memset(ctx, 0, sizeof *ctx);
            ctx->vtable = &br_x509_insecure_vtable;
            ctx->done_cert = false;
            ctx->match_fingerprint = _use_fingerprint ? _fingerprint : nullptr;
            ctx->allow_self_signed = _allow_self_signed ? 1 : 0;
        }

        void ESP8266_SSL_Client::setTimeout(unsigned long timeout)
        {
            this->_timeout = timeout;
            if (base_client)
                base_client->setTimeout(timeout);
        }

        void ESP8266_SSL_Client::setClient(Client *client)
        {
            base_client = client;
        }

        // Some constants uses to init the server/client contexts
        // Note that suites_P needs to be copied to RAM before use w/BearSSL!
        // List copied verbatim from BearSSL/ssl_client_full.c
        /*
            The "full" profile supports all implemented cipher suites.

            Rationale for suite order, from most important to least
            important rule:

            -- Don't use 3DES if AES or ChaCha20 is available.
            -- Try to have Forward Secrecy (ECDHE suite) if possible.
            -- When not using Forward Secrecy, ECDH key exchange is
              better than RSA key exchange (slightly more expensive on the
              client, but much cheaper on the server, and it implies smaller
              messages).
            -- ChaCha20+Poly1305 is better than AES/GCM (faster, smaller code).
            -- GCM is better than CCM and CBC. CCM is better than CBC.
            -- CCM is preferable over CCM_8 (with CCM_8, forgeries may succeed
              with probability 2^(-64)).
            -- AES-128 is preferred over AES-256 (AES-128 is already
              strong enough, and AES-256 is 40% more expensive).
        */
        static const uint16_t suites_P[] PROGMEM = {
#ifndef BEARSSL_SSL_BASIC
            BR_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256,
            BR_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CCM,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CCM,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,
            BR_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,
            BR_TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA,
            BR_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA,
            BR_TLS_RSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_RSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_RSA_WITH_AES_128_CCM,
            BR_TLS_RSA_WITH_AES_256_CCM,
            BR_TLS_RSA_WITH_AES_128_CCM_8,
            BR_TLS_RSA_WITH_AES_256_CCM_8,
#endif
            BR_TLS_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_RSA_WITH_AES_256_CBC_SHA256,
            BR_TLS_RSA_WITH_AES_128_CBC_SHA,
            BR_TLS_RSA_WITH_AES_256_CBC_SHA,
#ifndef BEARSSL_SSL_BASIC
            BR_TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_RSA_WITH_3DES_EDE_CBC_SHA
#endif
        };
#ifndef BEARSSL_SSL_BASIC
        // Server w/EC has one set, not possible with basic SSL config
        static const uint16_t suites_server_ec_P[] PROGMEM = {
            BR_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CCM,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CCM,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,
            BR_TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA,
            BR_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA,
            BR_TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA};
#endif

        static const uint16_t suites_server_rsa_P[] PROGMEM = {
#ifndef BEARSSL_SSL_BASIC
            BR_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
            BR_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,
            BR_TLS_RSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_RSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_RSA_WITH_AES_128_CCM,
            BR_TLS_RSA_WITH_AES_256_CCM,
            BR_TLS_RSA_WITH_AES_128_CCM_8,
            BR_TLS_RSA_WITH_AES_256_CCM_8,
#endif
            BR_TLS_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_RSA_WITH_AES_256_CBC_SHA256,
            BR_TLS_RSA_WITH_AES_128_CBC_SHA,
            BR_TLS_RSA_WITH_AES_256_CBC_SHA,
#ifndef BEARSSL_SSL_BASIC
            BR_TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_RSA_WITH_3DES_EDE_CBC_SHA
#endif
        };

        // For apps which want to use less secure but faster ciphers, only
        static const uint16_t faster_suites_P[] PROGMEM = {
            BR_TLS_RSA_WITH_AES_256_CBC_SHA256,
            BR_TLS_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_RSA_WITH_AES_256_CBC_SHA,
            BR_TLS_RSA_WITH_AES_128_CBC_SHA};

        // Install hashes into the SSL engine
        static void br_ssl_client_install_hashes(br_ssl_engine_context *eng)
        {
            br_ssl_engine_set_hash(eng, br_md5_ID, &br_md5_vtable);
            br_ssl_engine_set_hash(eng, br_sha1_ID, &br_sha1_vtable);
            br_ssl_engine_set_hash(eng, br_sha224_ID, &br_sha224_vtable);
            br_ssl_engine_set_hash(eng, br_sha256_ID, &br_sha256_vtable);
            br_ssl_engine_set_hash(eng, br_sha384_ID, &br_sha384_vtable);
            br_ssl_engine_set_hash(eng, br_sha512_ID, &br_sha512_vtable);
        }

        static void br_x509_minimal_install_hashes(br_x509_minimal_context *x509)
        {
            br_x509_minimal_set_hash(x509, br_md5_ID, &br_md5_vtable);
            br_x509_minimal_set_hash(x509, br_sha1_ID, &br_sha1_vtable);
            br_x509_minimal_set_hash(x509, br_sha224_ID, &br_sha224_vtable);
            br_x509_minimal_set_hash(x509, br_sha256_ID, &br_sha256_vtable);
            br_x509_minimal_set_hash(x509, br_sha384_ID, &br_sha384_vtable);
            br_x509_minimal_set_hash(x509, br_sha512_ID, &br_sha512_vtable);
        }

        // Default initializion for our SSL clients
        static void br_ssl_client_base_init(br_ssl_client_context *cc, const uint16_t *cipher_list, int cipher_cnt)
        {
            uint16_t suites[cipher_cnt];
            memcpy_P(suites, cipher_list, cipher_cnt * sizeof(cipher_list[0]));
            br_ssl_client_zero(cc);
            br_ssl_engine_add_flags(&cc->eng, BR_OPT_NO_RENEGOTIATION); // forbid SSL renegotiation, as we free the Private Key after handshake
            br_ssl_engine_set_versions(&cc->eng, BR_TLS10, BR_TLS12);
            br_ssl_engine_set_suites(&cc->eng, suites, (sizeof suites) / (sizeof suites[0]));
            br_ssl_client_set_default_rsapub(cc);
            br_ssl_engine_set_default_rsavrfy(&cc->eng);
#ifndef BEARSSL_SSL_BASIC
            br_ssl_engine_set_default_ecdsa(&cc->eng);
#endif
            br_ssl_client_install_hashes(&cc->eng);
            br_ssl_engine_set_prf10(&cc->eng, &br_tls10_prf);
            br_ssl_engine_set_prf_sha256(&cc->eng, &br_tls12_sha256_prf);
            br_ssl_engine_set_prf_sha384(&cc->eng, &br_tls12_sha384_prf);
            br_ssl_engine_set_default_aes_cbc(&cc->eng);
#ifndef BEARSSL_SSL_BASIC
            br_ssl_engine_set_default_aes_gcm(&cc->eng);
            br_ssl_engine_set_default_aes_ccm(&cc->eng);
            br_ssl_engine_set_default_des_cbc(&cc->eng);
            br_ssl_engine_set_default_chapol(&cc->eng);
#endif
        }

        // Default initializion for our SSL clients
        static void br_ssl_server_base_init(br_ssl_server_context *cc, const uint16_t *cipher_list, int cipher_cnt)
        {
            uint16_t suites[cipher_cnt];
            memcpy_P(suites, cipher_list, cipher_cnt * sizeof(cipher_list[0]));
            br_ssl_server_zero(cc);
            br_ssl_engine_add_flags(&cc->eng, BR_OPT_NO_RENEGOTIATION); // forbid SSL renegotiation, as we free the Private Key after handshake
            br_ssl_engine_set_versions(&cc->eng, BR_TLS10, BR_TLS12);
            br_ssl_engine_set_suites(&cc->eng, suites, (sizeof suites) / (sizeof suites[0]));
#ifndef BEARSSL_SSL_BASIC
            br_ssl_engine_set_default_ec(&cc->eng);
#endif

            br_ssl_client_install_hashes(&cc->eng);
            br_ssl_engine_set_prf10(&cc->eng, &br_tls10_prf);
            br_ssl_engine_set_prf_sha256(&cc->eng, &br_tls12_sha256_prf);
            br_ssl_engine_set_prf_sha384(&cc->eng, &br_tls12_sha384_prf);
            br_ssl_engine_set_default_aes_cbc(&cc->eng);
#ifndef BEARSSL_SSL_BASIC
            br_ssl_engine_set_default_aes_ccm(&cc->eng);
            br_ssl_engine_set_default_aes_gcm(&cc->eng);
            br_ssl_engine_set_default_des_cbc(&cc->eng);
            br_ssl_engine_set_default_chapol(&cc->eng);
#endif
        }
    }

    // Set custom list of ciphers
    bool ESP8266_SSL_Client::setCiphers(const uint16_t *cipherAry, int cipherCount)
    {
        _cipher_list = nullptr;
        _cipher_list = std::shared_ptr<uint16_t>(new (std::nothrow) uint16_t[cipherCount], std::default_delete<uint16_t[]>());
        if (!_cipher_list.get())
        {
            DEBUG_BSSL("setCiphers: list empty\n");
            return false;
        }
        memcpy_P(_cipher_list.get(), cipherAry, cipherCount * sizeof(uint16_t));
        _cipher_cnt = cipherCount;
        return true;
    }

    bool ESP8266_SSL_Client::setCiphersLessSecure()
    {
        return setCiphers(faster_suites_P, sizeof(faster_suites_P) / sizeof(faster_suites_P[0]));
    }

    bool ESP8266_SSL_Client::setCiphers(const std::vector<uint16_t> &list)
    {
        return setCiphers(&list[0], list.size());
    }

    bool ESP8266_SSL_Client::setSSLVersion(uint32_t min, uint32_t max)
    {
        if (((min != BR_TLS10) && (min != BR_TLS11) && (min != BR_TLS12)) ||
            ((max != BR_TLS10) && (max != BR_TLS11) && (max != BR_TLS12)) ||
            (max < min))
        {
            return false; // Invalid options
        }
        _tls_min = min;
        _tls_max = max;
        return true;
    }

    // Installs the appropriate X509 cert validation method for a client connection
    bool ESP8266_SSL_Client::_installClientX509Validator()
    {
        if (_use_insecure || _use_fingerprint || _use_self_signed)
        {
            // Use common insecure x509 authenticator
            _x509_insecure = std::make_shared<struct br_x509_insecure_context>();
            if (!_x509_insecure)
            {
                DEBUG_BSSL("_installClientX509Validator: OOM for _x509_insecure\n");
                return false;
            }
            _mb_bssl_br_x509_insecure_init(_x509_insecure.get(), _use_fingerprint, _fingerprint, _use_self_signed);
            br_ssl_engine_set_x509(_eng, &_x509_insecure->vtable);
        }
        else if (_knownkey)
        {
            // Simple, pre-known public key authenticator, ignores cert completely.
            _x509_knownkey = std::make_shared<br_x509_knownkey_context>();
            if (!_x509_knownkey)
            {
                DEBUG_BSSL("_installClientX509Validator: OOM for _x509_knownkey\n");
                return false;
            }
            if (_knownkey->isRSA())
            {
                br_x509_knownkey_init_rsa(_x509_knownkey.get(), _knownkey->getRSA(), _knownkey_usages);
            }
            else if (_knownkey->isEC())
            {
#ifndef BEARSSL_SSL_BASIC
                br_x509_knownkey_init_ec(_x509_knownkey.get(), _knownkey->getEC(), _knownkey_usages);
#else
                (void)_knownkey;
                (void)_knownkey_usages;
                DEBUG_BSSL("_installClientX509Validator: Attempting to use EC keys in minimal cipher mode (no EC)\n");
                return false;
#endif
            }
            br_ssl_engine_set_x509(_eng, &_x509_knownkey->vtable);
        }
        else
        {
            // X509 minimal validator.  Checks dates, cert chain for trusted CA, etc.
            _x509_minimal = std::make_shared<br_x509_minimal_context>();
            if (!_x509_minimal)
            {
                DEBUG_BSSL("_installClientX509Validator: OOM for _x509_minimal\n");
                return false;
            }
            if (_esp32_ta)
            {
                br_x509_minimal_init(_x509_minimal.get(), &br_sha256_vtable, _esp32_ta->getTrustAnchors(), _esp32_ta->getCount());
            }
            else
            {
                br_x509_minimal_init(_x509_minimal.get(), &br_sha256_vtable, _ta ? _ta->getTrustAnchors() : nullptr, _ta ? _ta->getCount() : 0);
            }
            br_x509_minimal_set_rsa(_x509_minimal.get(), br_ssl_engine_get_rsavrfy(_eng));
#ifndef BEARSSL_SSL_BASIC
            br_x509_minimal_set_ecdsa(_x509_minimal.get(), br_ssl_engine_get_ec(_eng), br_ssl_engine_get_ecdsa(_eng));
#endif
            br_x509_minimal_install_hashes(_x509_minimal.get());
            if (_now)
            {
                // Magic constants convert to x509 times
                br_x509_minimal_set_time(_x509_minimal.get(), ((uint32_t)_now) / 86400 + 719528, ((uint32_t)_now) % 86400);
            }
            if (_certStore)
            {
                _certStore->installCertStore(_x509_minimal.get());
            }
            br_ssl_engine_set_x509(_eng, &_x509_minimal->vtable);
        }
        return true;
    }

    std::shared_ptr<unsigned char> ESP8266_SSL_Client::_alloc_iobuf(size_t sz)
    {
#if defined(ESP8266)

        // Allocate buffer with preference to IRAM
#if defined(ESP8266_CORE_SDK_V3_X_X)
        HeapSelectIram primary;
        auto sptr = std::shared_ptr<unsigned char>(new (std::nothrow) unsigned char[sz], std::default_delete<unsigned char[]>());
        if (!sptr)
        {
            HeapSelectDram alternate;
            sptr = std::shared_ptr<unsigned char>(new (std::nothrow) unsigned char[sz], std::default_delete<unsigned char[]>());
        }
        return sptr;
#else
        return std::shared_ptr<unsigned char>(new unsigned char[sz], std::default_delete<unsigned char[]>());
#endif

#else
        // Allocate buffer with preference to IRAM
        //  HeapSelectIram primary;
        auto sptr = std::shared_ptr<unsigned char>(new (std::nothrow) unsigned char[sz], std::default_delete<unsigned char[]>());
        if (!sptr)
        {
            //    HeapSelectDram alternate;
            sptr = std::shared_ptr<unsigned char>(new (std::nothrow) unsigned char[sz], std::default_delete<unsigned char[]>());
        }
        return sptr;

#endif
    }

    // Called by connect() to do the actual SSL setup and handshake.
    // Returns if the SSL handshake succeeded.
    bool ESP8266_SSL_Client::_connectSSL(const char *hostName)
    {
        if (!base_client)
        {
            if (debugLevel > 0)
                ESP_MAIL_PRINTF((const char *)FPSTR("> E: Client is not yet initialized\n"));
            return false;
        }

        if (!_clientConnected())
        {
            if (debugLevel > 0)
                ESP_MAIL_PRINTF((const char *)FPSTR("> E: Client is not connected\n"));
            return false;
        }

        DEBUG_BSSL("m_connectSSL: start connection\n");
        _freeSSL();
        _oom_err = false;

#if defined(DEBUG_ESP_SSL)
        // BearSSL will reject all connections unless an authentication option is set, warn in DEBUG builds
        if (!_use_insecure && !_use_fingerprint && !_use_self_signed && !_knownkey && !_certStore && !_ta)
        {
            DEBUG_BSSL("Connection *will* fail, no authentication method is setup\n");
        }
#endif
        _sc = std::make_shared<br_ssl_client_context>();
        _eng = &_sc->eng; // Allocation/deallocation taken care of by the _sc shared_ptr
        _iobuf_in = _alloc_iobuf(_iobuf_in_size);
        _iobuf_out = _alloc_iobuf(_iobuf_out_size);
        //  DBG_MMU_PRINTF("\n_iobuf_in:       %p\n", _iobuf_in.get());
        //  DBG_MMU_PRINTF(  "_iobuf_out:      %p\n", _iobuf_out.get());
        //  DBG_MMU_PRINTF(  "_iobuf_in_size:  %u\n", _iobuf_in_size);
        //  DBG_MMU_PRINTF(  "_iobuf_out_size: %u\n", _iobuf_out_size);

        if (!_sc || !_iobuf_in || !_iobuf_out)
        {
            _freeSSL(); // Frees _sc, _iobuf*
            _oom_err = true;
            DEBUG_BSSL("_connectSSL: OOM error\n");
            return false;
        }

        // If no cipher list yet set, use defaults
        if (_cipher_list.get() == nullptr)
        {
            br_ssl_client_base_init(_sc.get(), suites_P, sizeof(suites_P) / sizeof(suites_P[0]));
        }
        else
        {
            br_ssl_client_base_init(_sc.get(), _cipher_list.get(), _cipher_cnt);
        }
        // Only failure possible in the installation is OOM
        if (!_installClientX509Validator())
        {
            _freeSSL();
            _oom_err = true;
            DEBUG_BSSL("_connectSSL: Can't install x509 validator\n");
            return false;
        }
        br_ssl_engine_set_buffers_bidi(_eng, _iobuf_in.get(), _iobuf_in_size, _iobuf_out.get(), _iobuf_out_size);
        br_ssl_engine_set_versions(_eng, _tls_min, _tls_max);

        // Apply any client certificates, if supplied.
        if (_sk && _sk->isRSA())
        {
            br_ssl_client_set_single_rsa(_sc.get(), _chain ? _chain->getX509Certs() : nullptr, _chain ? _chain->getCount() : 0,
                                         _sk->getRSA(), br_rsa_pkcs1_sign_get_default());
        }
        else if (_sk && _sk->isEC())
        {
#ifndef BEARSSL_SSL_BASIC
            br_ssl_client_set_single_ec(_sc.get(), _chain ? _chain->getX509Certs() : nullptr, _chain ? _chain->getCount() : 0,
                                        _sk->getEC(), _allowed_usages,
                                        _cert_issuer_key_type, br_ec_get_default(), br_ecdsa_sign_asn1_get_default());
#else
            _freeSSL();
            DEBUG_BSSL("connectSSL: Attempting to use EC cert in minimal cipher mode (no EC)\n");
            return false;
#endif
        }
        else if (_esp32_sk && _esp32_chain)
        {
            br_ssl_client_set_single_rsa(_sc.get(), _esp32_chain->getX509Certs(), _esp32_chain->getCount(),
                                         _esp32_sk->getRSA(), br_rsa_pkcs1_sign_get_default());
        }

        // Restore session from the storage spot, if present
        if (_session)
        {
            br_ssl_engine_set_session_parameters(_eng, _session->getSession());
        }

        if (!br_ssl_client_reset(_sc.get(), this->host.c_str(), _session ? 1 : 0))
        {
            _freeSSL();
            DEBUG_BSSL("connectSSL: Can't reset client\n");
            return false;
        }

        auto ret = _wait_for_handshake();
#ifdef DEBUG_ESP_SSL
        if (!ret)
        {
            char err[256];
            getLastSSLError(err, sizeof(err));
            DEBUG_BSSL("Couldn't connect. Error = '%s'\n", err);
        }
        else
        {
            DEBUG_BSSL("Connected!\n");
        }
#endif

        // Session is already validated here, there is no need to keep following
        _x509_minimal = nullptr;
        _x509_insecure = nullptr;
        _x509_knownkey = nullptr;

        secured = ret;

        return ret;
    }

    // Slightly different X509 setup for servers who want to validate client
    // certificates, so factor it out as it's used in RSA and EC servers.
    bool ESP8266_SSL_Client::_installServerX509Validator(const X509List *client_CA_ta)
    {
        if (client_CA_ta)
        {
            _ta = client_CA_ta;
            // X509 minimal validator.  Checks dates, cert chain for trusted CA, etc.
            _x509_minimal = std::make_shared<br_x509_minimal_context>();
            if (!_x509_minimal)
            {
                _freeSSL();
                _oom_err = true;
                DEBUG_BSSL("_installServerX509Validator: OOM for _x509_minimal\n");
                return false;
            }
            br_x509_minimal_init(_x509_minimal.get(), &br_sha256_vtable, _ta->getTrustAnchors(), _ta->getCount());
            br_ssl_engine_set_default_rsavrfy(_eng);
#ifndef BEARSSL_SSL_BASIC
            br_ssl_engine_set_default_ecdsa(_eng);
#endif
            br_x509_minimal_set_rsa(_x509_minimal.get(), br_ssl_engine_get_rsavrfy(_eng));
#ifndef BEARSSL_SSL_BASIC
            br_x509_minimal_set_ecdsa(_x509_minimal.get(), br_ssl_engine_get_ec(_eng), br_ssl_engine_get_ecdsa(_eng));
#endif
            br_x509_minimal_install_hashes(_x509_minimal.get());
            if (_now)
            {
                // Magic constants convert to x509 times
                br_x509_minimal_set_time(_x509_minimal.get(), ((uint32_t)_now) / 86400 + 719528, ((uint32_t)_now) % 86400);
            }
            br_ssl_engine_set_x509(_eng, &_x509_minimal->vtable);
            br_ssl_server_set_trust_anchor_names_alt(_sc_svr.get(), _ta->getTrustAnchors(), _ta->getCount());
        }
        return true;
    }
    // Called by WiFiServerBearSSL when an RSA cert/key is specified.
    bool ESP8266_SSL_Client::_connectSSLServerRSA(const X509List *chain,
                                                  const PrivateKey *sk, _BearSSL_ServerSessions *cache,
                                                  const X509List *client_CA_ta)
    {
        _freeSSL();
        _oom_err = false;
        _sc_svr = std::make_shared<br_ssl_server_context>();
        _eng = &_sc_svr->eng; // Allocation/deallocation taken care of by the _sc shared_ptr
        _iobuf_in = _alloc_iobuf(_iobuf_in_size);
        _iobuf_out = _alloc_iobuf(_iobuf_out_size);
        //  DBG_MMU_PRINTF("\n_iobuf_in:       %p\n", _iobuf_in.get());
        //  DBG_MMU_PRINTF(  "_iobuf_out:      %p\n", _iobuf_out.get());
        //  DBG_MMU_PRINTF(  "_iobuf_in_size:  %u\n", _iobuf_in_size);
        //  DBG_MMU_PRINTF(  "_iobuf_out_size: %u\n", _iobuf_out_size);

        if (!_sc_svr || !_iobuf_in || !_iobuf_out)
        {
            _freeSSL();
            _oom_err = true;
            DEBUG_BSSL("_connectSSLServerRSA: OOM error\n");
            return false;
        }

        br_ssl_server_base_init(_sc_svr.get(), suites_server_rsa_P, sizeof(suites_server_rsa_P) / sizeof(suites_server_rsa_P[0]));
        br_ssl_server_set_single_rsa(_sc_svr.get(), chain ? chain->getX509Certs() : nullptr, chain ? chain->getCount() : 0,
                                     sk ? sk->getRSA() : nullptr, BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN,
                                     br_rsa_private_get_default(), br_rsa_pkcs1_sign_get_default());
        br_ssl_engine_set_buffers_bidi(_eng, _iobuf_in.get(), _iobuf_in_size, _iobuf_out.get(), _iobuf_out_size);
        br_ssl_engine_set_versions(_eng, _tls_min, _tls_max);
        if (cache != nullptr)
        {
            br_ssl_server_set_cache(_sc_svr.get(), cache->getCache());
        }
        if (client_CA_ta && !_installServerX509Validator(client_CA_ta))
        {
            DEBUG_BSSL("_connectSSLServerRSA: Can't install serverX509check\n");
            return false;
        }
        if (!br_ssl_server_reset(_sc_svr.get()))
        {
            _freeSSL();
            DEBUG_BSSL("_connectSSLServerRSA: Can't reset server ctx\n");
            return false;
        }

        return _wait_for_handshake();
    }

    // Called by WiFiServerBearSSL when an elliptic curve cert/key is specified.
    bool ESP8266_SSL_Client::_connectSSLServerEC(const X509List *chain,
                                                 unsigned cert_issuer_key_type, const PrivateKey *sk,
                                                 _BearSSL_ServerSessions *cache, const X509List *client_CA_ta)
    {
#ifndef BEARSSL_SSL_BASIC
        _freeSSL();
        _oom_err = false;
        _sc_svr = std::make_shared<br_ssl_server_context>();
        _eng = &_sc_svr->eng; // Allocation/deallocation taken care of by the _sc shared_ptr
        _iobuf_in = _alloc_iobuf(_iobuf_in_size);
        _iobuf_out = _alloc_iobuf(_iobuf_out_size);
        //  DBG_MMU_PRINTF("\n_iobuf_in:       %p\n", _iobuf_in.get());
        //  DBG_MMU_PRINTF(  "_iobuf_out:      %p\n", _iobuf_out.get());
        //  DBG_MMU_PRINTF(  "_iobuf_in_size:  %u\n", _iobuf_in_size);
        //  DBG_MMU_PRINTF(  "_iobuf_out_size: %u\n", _iobuf_out_size);

        if (!_sc_svr || !_iobuf_in || !_iobuf_out)
        {
            _freeSSL();
            _oom_err = true;
            DEBUG_BSSL("_connectSSLServerEC: OOM error\n");
            return false;
        }

        br_ssl_server_base_init(_sc_svr.get(), suites_server_ec_P, sizeof(suites_server_ec_P) / sizeof(suites_server_ec_P[0]));
        br_ssl_server_set_single_ec(_sc_svr.get(), chain ? chain->getX509Certs() : nullptr, chain ? chain->getCount() : 0,
                                    sk ? sk->getEC() : nullptr, BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN,
                                    cert_issuer_key_type, br_ssl_engine_get_ec(_eng), br_ecdsa_i15_sign_asn1);
        br_ssl_engine_set_buffers_bidi(_eng, _iobuf_in.get(), _iobuf_in_size, _iobuf_out.get(), _iobuf_out_size);
        br_ssl_engine_set_versions(_eng, _tls_min, _tls_max);
        if (cache != nullptr)
        {
            br_ssl_server_set_cache(_sc_svr.get(), cache->getCache());
        }
        if (client_CA_ta && !_installServerX509Validator(client_CA_ta))
        {
            DEBUG_BSSL("_connectSSLServerEC: Can't install serverX509check\n");
            return false;
        }
        if (!br_ssl_server_reset(_sc_svr.get()))
        {
            _freeSSL();
            DEBUG_BSSL("_connectSSLServerEC: Can't reset server ctx\n");
            return false;
        }

        return _wait_for_handshake();
#else
        (void)chain;
        (void)cert_issuer_key_type;
        (void)sk;
        (void)cache;
        (void)client_CA_ta;
        DEBUG_BSSL("_connectSSLServerEC: Attempting to use EC cert in minimal cipher mode (no EC)\n");
        return false;
#endif
    }

    // Returns an error ID and possibly a string (if dest != null) of the last
    // BearSSL reported error.
    int ESP8266_SSL_Client::getLastSSLError(char *dest, size_t len)
    {
        int err = 0;
        const char *t = PSTR("OK");
        const char *recv_fatal = "";
        const char *send_fatal = "";
        if (_sc || _sc_svr)
        {
            err = br_ssl_engine_last_error(_eng);
        }
        if (_oom_err)
        {
            err = -1000;
        }
        else
        {
            if (err & BR_ERR_RECV_FATAL_ALERT)
            {
                recv_fatal = PSTR("SSL received fatal alert - ");
                err &= ~BR_ERR_RECV_FATAL_ALERT;
            }
            if (err & BR_ERR_SEND_FATAL_ALERT)
            {
                send_fatal = PSTR("SSL sent fatal alert - ");
                err &= ~BR_ERR_SEND_FATAL_ALERT;
            }
        }
        switch (err)
        {
        case -1000:
            t = PSTR("Unable to allocate memory for SSL structures and buffers.");
            break;
        case BR_ERR_BAD_PARAM:
            t = PSTR("Caller-provided parameter is incorrect.");
            break;
        case BR_ERR_BAD_STATE:
            t = PSTR("Operation requested by the caller cannot be applied with the current context state (e.g. reading data while outgoing data is waiting to be sent).");
            break;
        case BR_ERR_UNSUPPORTED_VERSION:
            t = PSTR("Incoming protocol or record version is unsupported.");
            break;
        case BR_ERR_BAD_VERSION:
            t = PSTR("Incoming record version does not match the expected version.");
            break;
        case BR_ERR_BAD_LENGTH:
            t = PSTR("Incoming record length is invalid.");
            break;
        case BR_ERR_TOO_LARGE:
            t = PSTR("Incoming record is too large to be processed, or buffer is too small for the handshake message to send.");
            break;
        case BR_ERR_BAD_MAC:
            t = PSTR("Decryption found an invalid padding, or the record MAC is not correct.");
            break;
        case BR_ERR_NO_RANDOM:
            t = PSTR("No initial entropy was provided, and none can be obtained from the OS.");
            break;
        case BR_ERR_UNKNOWN_TYPE:
            t = PSTR("Incoming record type is unknown.");
            break;
        case BR_ERR_UNEXPECTED:
            t = PSTR("Incoming record or message has wrong type with regards to the current engine state.");
            break;
        case BR_ERR_BAD_CCS:
            t = PSTR("ChangeCipherSpec message from the peer has invalid contents.");
            break;
        case BR_ERR_BAD_ALERT:
            t = PSTR("Alert message from the peer has invalid contents (odd length).");
            break;
        case BR_ERR_BAD_HANDSHAKE:
            t = PSTR("Incoming handshake message decoding failed.");
            break;
        case BR_ERR_OVERSIZED_ID:
            t = PSTR("ServerHello contains a session ID which is larger than 32 bytes.");
            break;
        case BR_ERR_BAD_CIPHER_SUITE:
            t = PSTR("Server wants to use a cipher suite that we did not claim to support. This is also reported if we tried to advertise a cipher suite that we do not support.");
            break;
        case BR_ERR_BAD_COMPRESSION:
            t = PSTR("Server wants to use a compression that we did not claim to support.");
            break;
        case BR_ERR_BAD_FRAGLEN:
            t = PSTR("Server's max fragment length does not match client's.");
            break;
        case BR_ERR_BAD_SECRENEG:
            t = PSTR("Secure renegotiation failed.");
            break;
        case BR_ERR_EXTRA_EXTENSION:
            t = PSTR("Server sent an extension type that we did not announce, or used the same extension type several times in a single ServerHello.");
            break;
        case BR_ERR_BAD_SNI:
            t = PSTR("Invalid Server Name Indication contents (when used by the server, this extension shall be empty).");
            break;
        case BR_ERR_BAD_HELLO_DONE:
            t = PSTR("Invalid ServerHelloDone from the server (length is not 0).");
            break;
        case BR_ERR_LIMIT_EXCEEDED:
            t = PSTR("Internal limit exceeded (e.g. server's public key is too large).");
            break;
        case BR_ERR_BAD_FINISHED:
            t = PSTR("Finished message from peer does not match the expected value.");
            break;
        case BR_ERR_RESUME_MISMATCH:
            t = PSTR("Session resumption attempt with distinct version or cipher suite.");
            break;
        case BR_ERR_INVALID_ALGORITHM:
            t = PSTR("Unsupported or invalid algorithm (ECDHE curve, signature algorithm, hash function).");
            break;
        case BR_ERR_BAD_SIGNATURE:
            t = PSTR("Invalid signature in ServerKeyExchange or CertificateVerify message.");
            break;
        case BR_ERR_WRONG_KEY_USAGE:
            t = PSTR("Peer's public key does not have the proper type or is not allowed for the requested operation.");
            break;
        case BR_ERR_NO_CLIENT_AUTH:
            t = PSTR("Client did not send a certificate upon request, or the client certificate could not be validated.");
            break;
        case BR_ERR_IO:
            t = PSTR("I/O error or premature close on transport stream.");
            break;
        case BR_ERR_X509_INVALID_VALUE:
            t = PSTR("Invalid value in an ASN.1 structure.");
            break;
        case BR_ERR_X509_TRUNCATED:
            t = PSTR("Truncated certificate or other ASN.1 object.");
            break;
        case BR_ERR_X509_EMPTY_CHAIN:
            t = PSTR("Empty certificate chain (no certificate at all).");
            break;
        case BR_ERR_X509_INNER_TRUNC:
            t = PSTR("Decoding error: inner element extends beyond outer element size.");
            break;
        case BR_ERR_X509_BAD_TAG_CLASS:
            t = PSTR("Decoding error: unsupported tag class (application or private).");
            break;
        case BR_ERR_X509_BAD_TAG_VALUE:
            t = PSTR("Decoding error: unsupported tag value.");
            break;
        case BR_ERR_X509_INDEFINITE_LENGTH:
            t = PSTR("Decoding error: indefinite length.");
            break;
        case BR_ERR_X509_EXTRA_ELEMENT:
            t = PSTR("Decoding error: extraneous element.");
            break;
        case BR_ERR_X509_UNEXPECTED:
            t = PSTR("Decoding error: unexpected element.");
            break;
        case BR_ERR_X509_NOT_CONSTRUCTED:
            t = PSTR("Decoding error: expected constructed element, but is primitive.");
            break;
        case BR_ERR_X509_NOT_PRIMITIVE:
            t = PSTR("Decoding error: expected primitive element, but is constructed.");
            break;
        case BR_ERR_X509_PARTIAL_BYTE:
            t = PSTR("Decoding error: BIT STRING length is not multiple of 8.");
            break;
        case BR_ERR_X509_BAD_BOOLEAN:
            t = PSTR("Decoding error: BOOLEAN value has invalid length.");
            break;
        case BR_ERR_X509_OVERFLOW:
            t = PSTR("Decoding error: value is off-limits.");
            break;
        case BR_ERR_X509_BAD_DN:
            t = PSTR("Invalid distinguished name.");
            break;
        case BR_ERR_X509_BAD_TIME:
            t = PSTR("Invalid date/time representation.");
            break;
        case BR_ERR_X509_UNSUPPORTED:
            t = PSTR("Certificate contains unsupported features that cannot be ignored.");
            break;
        case BR_ERR_X509_LIMIT_EXCEEDED:
            t = PSTR("Key or signature size exceeds internal limits.");
            break;
        case BR_ERR_X509_WRONG_KEY_TYPE:
            t = PSTR("Key type does not match that which was expected.");
            break;
        case BR_ERR_X509_BAD_SIGNATURE:
            t = PSTR("Signature is invalid.");
            break;
        case BR_ERR_X509_TIME_UNKNOWN:
            t = PSTR("Validation time is unknown.");
            break;
        case BR_ERR_X509_EXPIRED:
            t = PSTR("Certificate is expired or not yet valid.");
            break;
        case BR_ERR_X509_DN_MISMATCH:
            t = PSTR("Issuer/Subject DN mismatch in the chain.");
            break;
        case BR_ERR_X509_BAD_SERVER_NAME:
            t = PSTR("Expected server name was not found in the chain.");
            break;
        case BR_ERR_X509_CRITICAL_EXTENSION:
            t = PSTR("Unknown critical extension in certificate.");
            break;
        case BR_ERR_X509_NOT_CA:
            t = PSTR("Not a CA, or path length constraint violation.");
            break;
        case BR_ERR_X509_FORBIDDEN_KEY_USAGE:
            t = PSTR("Key Usage extension prohibits intended usage.");
            break;
        case BR_ERR_X509_WEAK_PUBLIC_KEY:
            t = PSTR("Public key found in certificate is too small.");
            break;
        case BR_ERR_X509_NOT_TRUSTED:
            t = PSTR("Chain could not be linked to a trust anchor.");
            break;
        default:
            t = PSTR("Unknown error code.");
            break;
        }
        if (dest)
        {
            // snprintf is PSTR safe and guaranteed to 0-terminate
            snprintf(dest, len, "%s%s%s", recv_fatal, send_fatal, t);
        }
        return err;
    }

    uint8_t *ESP8266_SSL_Client::_streamLoad(Stream &stream, size_t size)
    {
        uint8_t *dest = (uint8_t *)malloc(size + 1);
        if (!dest)
        {
            return nullptr;
        }
        if (size != stream.readBytes(dest, size))
        {
            free(dest);
            dest = nullptr;
            return nullptr;
        }
        dest[size] = '\0';
        return dest;
    }

    void ESP8266_SSL_Client::idle()
    {
#if defined(ARDUINO_ESP8266_MAJOR) && defined(ARDUINO_ESP8266_MINOR) && defined(ARDUINO_ESP8266_REVISION) && ((ARDUINO_ESP8266_MAJOR == 3 && ARDUINO_ESP8266_MINOR >= 1) || ARDUINO_ESP8266_MAJOR > 3)
        esp_yield();
#else
        delay(0);
#endif
    }
};
#endif // Internal SSL engine for basic clients

#endif // ESP8266

#endif // ESP8266_SSL_Client_CPP