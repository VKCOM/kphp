// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/curl.h"

#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/multi.h>

#include "runtime/critical_section.h"
#include "runtime/global_storage.h"
#include "runtime/interface.h"
#include "runtime/openssl.h"
#include "common/smart_ptrs/singleton.h"
#include "common/wrappers/to_array.h"

static_assert(LIBCURL_VERSION_NUM >= 0x071c00, "Outdated libcurl");
static_assert(CURL_MAX_WRITE_SIZE <= (1 << 30), "CURL_MAX_WRITE_SIZE expected to be less than (1 << 30)");

static_assert(CURLE_OK == 0, "check value");

static_assert(CURLM_CALL_MULTI_PERFORM == -1, "check value");
static_assert(CURLM_OK == 0, "check value");
static_assert(CURLM_BAD_HANDLE == 1, "check value");
static_assert(CURLM_BAD_EASY_HANDLE == 2, "check value");
static_assert(CURLM_OUT_OF_MEMORY == 3, "check value");
static_assert(CURLM_INTERNAL_ERROR == 4, "check value");
static_assert(CURLM_BAD_SOCKET == 5, "check value");
static_assert(CURLM_UNKNOWN_OPTION == 6, "check value");
static_assert(CURLM_ADDED_ALREADY == 7, "check value");

namespace {

constexpr int64_t BAD_CURL_OPTION = static_cast<int>(CURL_LAST) + static_cast<int>(CURL_FORMADD_LAST);

size_t curl_write(char *data, size_t size, size_t nmemb, void *userdata);

class BaseContext {
public:
  int64_t error_num{0};
  char error_msg[CURL_ERROR_SIZE + 1]{'\0'};

  template<size_t OPTION_OFFSET, size_t OPTIONS_COUNT>
  bool check_option_value(long opt_value, const char *what, const char *function, bool save_error = true) noexcept {
    if (unlikely(opt_value < static_cast<long>(OPTION_OFFSET) || opt_value - OPTION_OFFSET >= OPTIONS_COUNT)) {
      php_warning("Wrong %s %ld in function %s", what, opt_value, function);
      if (save_error) {
        error_num = BAD_CURL_OPTION;
        snprintf(error_msg, CURL_ERROR_SIZE, "Wrong %s %ld", what, opt_value);
      }
      return false;
    }
    return true;
  }

protected:
  ~BaseContext() = default;
};

class EasyContext : public BaseContext {
public:
  explicit EasyContext(CURL *handle, int64_t self_handler_id) noexcept:
    easy_handle(handle),
    self_id(self_handler_id) {
  }

  template<typename T>
  void set_option(CURLoption option, const T &value) noexcept {
    const CURLcode res = dl::critical_section_call([&] { return curl_easy_setopt(easy_handle, option, value); });
    php_assert (res == CURLE_OK);
  }

  template<typename T>
  int64_t set_option_safe(CURLoption option, const T &value) noexcept {
    dl::CriticalSectionGuard critical_section;
    error_num = curl_easy_setopt(easy_handle, option, value);
    return error_num;
  }

  mixed get_info(CURLINFO what) noexcept {
    if (what == CURLINFO_PRIVATE) {
      return private_data;
    }
    const int type = CURLINFO_TYPEMASK & what;
    switch (type) {
      case CURLINFO_STRING: {
        char *value = nullptr;
        const CURLcode res = dl::critical_section_call([&] { return curl_easy_getinfo (easy_handle, what, &value); });
        return (res == CURLE_OK && value) ? string{value} : mixed{false};
      }
      case CURLINFO_LONG: {
        long value = 0;
        const CURLcode res = dl::critical_section_call([&] { return curl_easy_getinfo (easy_handle, what, &value); });
        return res == CURLE_OK ? mixed{value} : mixed{false};
      }
      case CURLINFO_DOUBLE: {
        double value = 0;
        const CURLcode res = dl::critical_section_call([&] { return curl_easy_getinfo (easy_handle, what, &value); });
        return res == CURLE_OK ? mixed{value} : mixed{false};
      }
      default:
        php_critical_error("Got unknown curl info type '%d'", type);
        __builtin_unreachable();
    }
  }

  void add_info_into_array(array<mixed> &out, const char *key, CURLINFO what) noexcept {
    mixed value = get_info(what);
    if (!equals(value, false)) {
      out.set_value(string{key}, std::move(value));
    }
  }

  void set_default_options() noexcept {
    set_option(CURLOPT_NOPROGRESS, 1L);
    set_option(CURLOPT_VERBOSE, 0L);
    set_option(CURLOPT_ERRORBUFFER, error_msg);
    set_option(CURLOPT_WRITEFUNCTION, curl_write);
    set_option(CURLOPT_WRITEDATA, static_cast<void *>(this));
    set_option(CURLOPT_DNS_USE_GLOBAL_CACHE, 1L);
    set_option(CURLOPT_DNS_CACHE_TIMEOUT, 120L);
    set_option(CURLOPT_MAXREDIRS, 20L);
    set_option(CURLOPT_NOSIGNAL, 1L);
    set_option(CURLOPT_PRIVATE, reinterpret_cast<void *>(self_id));

    // Always disabled FILE and SCP
    set_option(CURLOPT_PROTOCOLS, static_cast<long>(CURLPROTO_ALL & ~(CURLPROTO_FILE | CURLPROTO_SCP)));
  }

  void cleanup_for_next_request() noexcept {
    header = string{};
    result = string{};
    error_num = 0;
    error_msg[0] = '\0';
  }

  void cleanup_slists_and_posts() noexcept {
    const auto last_slist = slists_to_free.cend();
    for (auto p = slists_to_free.cbegin(); p != last_slist; ++p) {
      curl_slist *v = p.get_value();
      php_assert (v != nullptr);
      dl::critical_section_call(curl_slist_free_all, v);
    }
    slists_to_free = array<curl_slist *>{};

    const auto last_post = httpposts_to_free.cend();
    for (auto p = httpposts_to_free.cbegin(); p != last_post; ++p) {
      curl_httppost *v = p.get_value();
      php_assert (v != nullptr);
      dl::critical_section_call(curl_formfree, v);
    }
    httpposts_to_free = array<curl_httppost *>{};
  }

  CURL *easy_handle;
  const int64_t self_id{-1};

  string header;
  string result;

  array<curl_slist *> slists_to_free;
  array<curl_httppost *> httpposts_to_free;
  Optional<string> private_data{false};

  bool return_transfer{false};
};

class MultiContext : public BaseContext {
public:
  explicit MultiContext(CURLM *handle) noexcept :
    multi_handle(handle) {
  }

  template<typename T>
  int64_t set_option_safe(CURLMoption option, const T &value) noexcept {
    dl::CriticalSectionGuard critical_section;
    error_num = curl_multi_setopt(multi_handle, option, value);
    return error_num;
  }

  CURLM *multi_handle;
};

struct CurlContexts_ {
  array<EasyContext *> easy_contexts;
  array<MultiContext *> multi_contexts;

  template<typename T>
  T *get_value(int64_t id) const noexcept;
};

template<>
EasyContext *CurlContexts_::get_value<EasyContext>(int64_t easy_id) const noexcept {
  return easy_contexts.get_value(easy_id - 1);
}

template<>
MultiContext *CurlContexts_::get_value<MultiContext>(int64_t multi_id) const noexcept {
  return multi_contexts.get_value(multi_id - 1);
}

using CurlContexts = vk::singleton<GlobalStorage<CurlContexts_>>;

template<typename T>
T *get_context(int64_t id) noexcept {
  T *context = nullptr;
  if (likely(dl::query_num == CurlContexts::get().get_query_tag())) {
    context = CurlContexts::get()->get_value<T>(id);
  }

  if (unlikely(!context)) {
    php_warning("Wrong context id specified");
  }
  return context;
}

void easy_close(EasyContext *easy_context) noexcept {
  dl::critical_section_call(curl_easy_cleanup, easy_context->easy_handle);
  easy_context->cleanup_slists_and_posts();
  easy_context->~EasyContext();
  dl::deallocate(easy_context, sizeof(EasyContext));
}

void multi_close(MultiContext *multi_context) noexcept {
  dl::critical_section_call(curl_multi_cleanup, multi_context->multi_handle);
  multi_context->~MultiContext();
  dl::deallocate(multi_context, sizeof(MultiContext));
}

// this is a callback called from curl_easy_perform
size_t curl_write(char *data, size_t size, size_t nmemb, void *userdata) {
  dl::leave_critical_section();

  auto length = static_cast<string::size_type>(size * nmemb);
  auto *easy_context = static_cast<EasyContext *>(userdata);

  if (easy_context->return_transfer) {
    easy_context->result.append(data, length);
  } else {
    print(data, length);
  }

  dl::enter_critical_section();
  return size * nmemb;
}

// this is a callback called from curl_easy_perform
int64_t curl_info_header_out(CURL *, curl_infotype type, char *buf, size_t buf_len, void *userdata) {
  if (type == CURLINFO_HEADER_OUT) {
    dl::leave_critical_section();
    static_cast<EasyContext *>(userdata)->header = string{buf, static_cast<string::size_type>(buf_len)};
    dl::enter_critical_section();
  }
  return 0;
}

void long_option_setter(EasyContext *easy_context, CURLoption option, const mixed &value) {
  easy_context->set_option_safe(option, static_cast<long>(value.to_int()));
}

void string_option_setter(EasyContext *easy_context, CURLoption option, const mixed &value) {
  easy_context->set_option_safe(option, value.to_string().c_str());
}

void off_option_setter(EasyContext *easy_context, CURLoption option, const mixed &value) {
  easy_context->set_option_safe(option, static_cast<curl_off_t>(value.to_int()));
}

void linked_list_option_setter(EasyContext *easy_context, CURLoption option, const mixed &value) {
  if (unlikely(!value.is_array())) {
    php_warning("Value must be an array in function curl_setopt");
    easy_context->error_num = BAD_CURL_OPTION;
    snprintf(easy_context->error_msg, CURL_ERROR_SIZE, "Value must be an array, type of value is %s", value.get_type_c_str());
    return;
  }

  const array<mixed> &v = value.to_array();
  curl_slist *slist = nullptr;
  for (auto p = v.begin(); p != v.end(); ++p) {
    slist = dl::critical_section_call(curl_slist_append, slist, f$strval(p.get_value()).c_str());
    if (unlikely(!slist)) {
      php_warning("Can't build curl_slist. How is it possible?");
      easy_context->error_num = BAD_CURL_OPTION;
      snprintf(easy_context->error_msg, CURL_ERROR_SIZE, "Can't build curl_slist. How is it possible?");
      return;
    }
  }

  if (slist) {
    easy_context->slists_to_free.push_back(slist);
    easy_context->set_option_safe(option, slist);
  }
}

void private_option_setter(EasyContext *easy_context, CURLoption option, const mixed &value) {
  php_assert(option == CURLOPT_PRIVATE);
  easy_context->private_data = value.to_string();
}

template<size_t OPTION_OFFSET, typename T, size_t N>
void set_enumerated_option(const std::array<T, N> &options, EasyContext *easy_context, CURLoption option, const mixed &value) noexcept {
  long val = static_cast<long>(value.to_int());
  if (easy_context->check_option_value<OPTION_OFFSET, N>(val, "parameter value", "curl_setopt")) {
    val = options[val - OPTION_OFFSET];
    easy_context->set_option_safe(option, val);
  }
}

void proxy_type_option_setter(EasyContext *easy_context, CURLoption option, const mixed &value) {
  constexpr static auto options = vk::to_array(
    {
      CURLPROXY_HTTP,
      CURLPROXY_HTTP_1_0,
      CURLPROXY_SOCKS4,
      CURLPROXY_SOCKS5,
      CURLPROXY_SOCKS4A,
      CURLPROXY_SOCKS5_HOSTNAME
    });
  set_enumerated_option<400000>(options, easy_context, option, value);
}

void ssl_version_option_setter(EasyContext *easy_context, CURLoption option, const mixed &value) {
  constexpr static auto options = vk::to_array(
    {
      CURL_SSLVERSION_DEFAULT,
      CURL_SSLVERSION_TLSv1,
      CURL_SSLVERSION_SSLv2,
      CURL_SSLVERSION_SSLv3,
      CURL_SSLVERSION_TLSv1_0,
      CURL_SSLVERSION_TLSv1_1,
      CURL_SSLVERSION_TLSv1_2,
    });
  set_enumerated_option<0>(options, easy_context, option, value);
}

void auth_option_setter(EasyContext *easy_context, CURLoption option, const mixed &value) {
  long val = static_cast<long>(value.to_int());
  constexpr size_t OPTION_OFFSET = 600000;
  if (easy_context->check_option_value<OPTION_OFFSET, 1u << 4u>(val, "parameter value", "curl_setopt")) {
    val -= OPTION_OFFSET;
    val = (val & 1) * CURLAUTH_BASIC +
          ((val >> 1) & 1) * CURLAUTH_DIGEST +
          // curl-kphp-vk doesn't support this option
          // ((val >> 2) & 1) * CURLAUTH_GSSNEGOTIATE +
          ((val >> 3) & 1) * CURLAUTH_NTLM;
    easy_context->set_option_safe(option, val);
  }
}

void ip_resolve_option_setter(EasyContext *easy_context, CURLoption option, const mixed &value) {
  constexpr static auto options = vk::to_array({CURL_IPRESOLVE_WHATEVER, CURL_IPRESOLVE_V4, CURL_IPRESOLVE_V6});
  set_enumerated_option<700000>(options, easy_context, option, value);
}

void ftp_auth_option_setter(EasyContext *easy_context, CURLoption option, const mixed &value) {
  constexpr static auto options = vk::to_array({CURLFTPAUTH_DEFAULT, CURLFTPAUTH_SSL, CURLFTPAUTH_TLS});
  set_enumerated_option<800000>(options, easy_context, option, value);
}

void ftp_method_option_setter(EasyContext *easy_context, CURLoption option, const mixed &value) {
  constexpr static auto options = vk::to_array({CURLFTPMETHOD_MULTICWD, CURLFTPMETHOD_NOCWD, CURLFTPMETHOD_SINGLECWD});
  set_enumerated_option<900000>(options, easy_context, option, value);
}

void post_fields_option_setter(EasyContext *easy_context, CURLoption, const mixed &value) {
  if (value.is_array()) {
    const array<mixed> &postfields = value.to_array();

    curl_httppost *first = nullptr;
    curl_httppost *last = nullptr;

    for (auto p = postfields.begin(); p != postfields.end(); ++p) {
      const string key = f$strval(p.get_key());
      const string v = f$strval(p.get_value());

      if (unlikely(v[0] == '@')) {
        php_critical_error ("files posting is not supported in curl_setopt with CURLOPT_POSTFIELDS\n");
      }

      dl::CriticalSectionGuard critical_section;
      easy_context->error_num = curl_formadd(&first, &last,
                                             CURLFORM_COPYNAME, key.c_str(),
                                             CURLFORM_NAMELENGTH, static_cast<long>(key.size()),
                                             CURLFORM_COPYCONTENTS, v.c_str(),
                                             CURLFORM_CONTENTSLENGTH, static_cast<long>(v.size()),
                                             CURLFORM_END);
      if (easy_context->error_num != 0) {
        easy_context->error_num += CURL_LAST;
        break;
      }
    }

    if (unlikely(easy_context->error_num != CURLE_OK)) {
      dl::critical_section_call(curl_formfree, first);
      php_warning("Call to curl_formadd has failed");
      easy_context->error_num = BAD_CURL_OPTION;
      snprintf(easy_context->error_msg, CURL_ERROR_SIZE, "Call to curl_formadd has failed");
      return;
    }

    if (first != nullptr) {
      easy_context->httpposts_to_free.push_back(first);
      easy_context->set_option_safe(CURLOPT_HTTPPOST, first);
    }
  } else {
    string post = value.to_string();
    easy_context->set_option(CURLOPT_POSTFIELDSIZE, post.size());
    easy_context->set_option_safe(CURLOPT_COPYPOSTFIELDS, post.c_str());
  }
}

constexpr int64_t CURLOPT_RETURNTRANSFER = 1234567;
constexpr int64_t CURLOPT_INFO_HEADER_OUT = 7654321;

bool curl_setopt(EasyContext *easy_context, int64_t option, const mixed &value) noexcept {
  if (option == CURLOPT_INFO_HEADER_OUT) {
    if (value.to_int() == 1ll) {
      easy_context->set_option(CURLOPT_DEBUGFUNCTION, curl_info_header_out);
      easy_context->set_option(CURLOPT_DEBUGDATA, static_cast<void *>(easy_context));
      easy_context->set_option(CURLOPT_VERBOSE, 1);
    } else {
      easy_context->set_option(CURLOPT_DEBUGFUNCTION, nullptr);
      easy_context->set_option(CURLOPT_DEBUGDATA, nullptr);
      easy_context->set_option(CURLOPT_VERBOSE, 0);
    }
    return true;
  }

  if (option == CURLOPT_RETURNTRANSFER) {
    easy_context->return_transfer = (value.to_int() == 1ll);
    return true;
  }

  struct EasyOptionHandler {
    CURLoption option;
    void (*option_setter)(EasyContext *easy_context, CURLoption option, const mixed &value);
  };
  constexpr static auto curlopt_options = vk::to_array<EasyOptionHandler>(
    {
      {CURLOPT_ADDRESS_SCOPE,        long_option_setter},
      {CURLOPT_APPEND,               long_option_setter},
      {CURLOPT_AUTOREFERER,          long_option_setter},
      {CURLOPT_BUFFERSIZE,           long_option_setter},
      {CURLOPT_CONNECT_ONLY,         long_option_setter},
      {CURLOPT_CONNECTTIMEOUT,       long_option_setter},
      {CURLOPT_CONNECTTIMEOUT_MS,    long_option_setter},
      {CURLOPT_COOKIESESSION,        long_option_setter},
      {CURLOPT_CRLF,                 long_option_setter},
      {CURLOPT_DIRLISTONLY,          long_option_setter},
      {CURLOPT_DNS_CACHE_TIMEOUT,    long_option_setter},
      {CURLOPT_FAILONERROR,          long_option_setter},
      {CURLOPT_FILETIME,             long_option_setter},
      {CURLOPT_FOLLOWLOCATION,       long_option_setter},
      {CURLOPT_FORBID_REUSE,         long_option_setter},
      {CURLOPT_FRESH_CONNECT,        long_option_setter},
      {CURLOPT_FTP_CREATE_MISSING_DIRS, long_option_setter},
      {CURLOPT_FTP_RESPONSE_TIMEOUT,    long_option_setter},
      {CURLOPT_FTP_SKIP_PASV_IP,        long_option_setter},
      {CURLOPT_FTP_USE_EPRT,            long_option_setter},
      {CURLOPT_FTP_USE_EPSV,            long_option_setter},
      {CURLOPT_FTP_USE_PRET,            long_option_setter},
      {CURLOPT_HEADER,                  long_option_setter},
      {CURLOPT_HTTP_CONTENT_DECODING,   long_option_setter},
      {CURLOPT_HTTP_TRANSFER_DECODING,  long_option_setter},
      {CURLOPT_HTTPGET,                 long_option_setter},
      {CURLOPT_HTTPPROXYTUNNEL,         long_option_setter},
      {CURLOPT_IGNORE_CONTENT_LENGTH,   long_option_setter},
      {CURLOPT_INFILESIZE,              long_option_setter},
      {CURLOPT_LOW_SPEED_LIMIT,         long_option_setter},
      {CURLOPT_LOW_SPEED_TIME,          long_option_setter},
      {CURLOPT_MAXCONNECTS,             long_option_setter},
      {CURLOPT_MAXFILESIZE,             long_option_setter},
      {CURLOPT_MAXREDIRS,               long_option_setter},
      {CURLOPT_NETRC,                   long_option_setter},
      {CURLOPT_NEW_DIRECTORY_PERMS,     long_option_setter},
      {CURLOPT_NEW_FILE_PERMS,          long_option_setter},
      {CURLOPT_NOBODY,                  long_option_setter},
      {CURLOPT_PORT,                    long_option_setter},
      {CURLOPT_POST,                    long_option_setter},
      {CURLOPT_PROXY_TRANSFER_MODE,     long_option_setter},
      {CURLOPT_PROXYPORT,               long_option_setter},
      {CURLOPT_RESUME_FROM,             long_option_setter},
      {CURLOPT_SOCKS5_GSSAPI_NEC,       long_option_setter},
      {CURLOPT_SSL_SESSIONID_CACHE,     long_option_setter},
      {CURLOPT_SSL_VERIFYHOST,          long_option_setter},
      {CURLOPT_SSL_VERIFYPEER,          long_option_setter},
      {CURLOPT_TCP_NODELAY,             long_option_setter},
      {CURLOPT_TFTP_BLKSIZE,            long_option_setter},
      {CURLOPT_TIMEOUT,                 long_option_setter},
      {CURLOPT_TIMEOUT_MS,              long_option_setter},
      {CURLOPT_TRANSFERTEXT,            long_option_setter},
      {CURLOPT_UNRESTRICTED_AUTH,       long_option_setter},
      {CURLOPT_UPLOAD,                  long_option_setter},
      {CURLOPT_VERBOSE,                 long_option_setter},
      {CURLOPT_WILDCARDMATCH,           long_option_setter},

      {CURLOPT_PROXYTYPE,               proxy_type_option_setter},

      {CURLOPT_SSLVERSION,              ssl_version_option_setter},

      {CURLOPT_HTTPAUTH,                auth_option_setter},
      {CURLOPT_PROXYAUTH,               auth_option_setter},

      {CURLOPT_IPRESOLVE,               ip_resolve_option_setter},

      {CURLOPT_FTPSSLAUTH,              ftp_auth_option_setter},

      {CURLOPT_FTP_FILEMETHOD,          ftp_method_option_setter},

      {CURLOPT_CAINFO,                  string_option_setter},
      {CURLOPT_CAPATH,                  string_option_setter},
      {CURLOPT_COOKIE,                  string_option_setter},
      {CURLOPT_COOKIEFILE,              string_option_setter},
      {CURLOPT_COOKIEJAR,               string_option_setter},
      {CURLOPT_COOKIELIST,              string_option_setter},
      {CURLOPT_CRLFILE,                 string_option_setter},
      {CURLOPT_CUSTOMREQUEST,           string_option_setter},
      {CURLOPT_EGDSOCKET,               string_option_setter},
      {CURLOPT_FTP_ACCOUNT,             string_option_setter},
      {CURLOPT_FTP_ALTERNATIVE_TO_USER, string_option_setter},
      {CURLOPT_FTPPORT,                 string_option_setter},
      {CURLOPT_INTERFACE,               string_option_setter},
      {CURLOPT_ISSUERCERT,              string_option_setter},
      {CURLOPT_KRBLEVEL,                string_option_setter},
      {CURLOPT_MAIL_FROM,               string_option_setter},
      {CURLOPT_NETRC_FILE,              string_option_setter},
      {CURLOPT_NOPROXY,                 string_option_setter},
      {CURLOPT_PASSWORD,                string_option_setter},
      {CURLOPT_PROXY,                   string_option_setter},
      {CURLOPT_PROXYPASSWORD,           string_option_setter},
      {CURLOPT_PROXYUSERNAME,           string_option_setter},
      {CURLOPT_PROXYUSERPWD,            string_option_setter},
      {CURLOPT_RANDOM_FILE,             string_option_setter},
      {CURLOPT_RANGE,                   string_option_setter},
      {CURLOPT_REFERER,                 string_option_setter},
      {CURLOPT_RTSP_SESSION_ID,         string_option_setter},
      {CURLOPT_RTSP_STREAM_URI,         string_option_setter},
      {CURLOPT_RTSP_TRANSPORT,          string_option_setter},
      {CURLOPT_SOCKS5_GSSAPI_SERVICE,   string_option_setter},
      {CURLOPT_SSH_HOST_PUBLIC_KEY_MD5, string_option_setter},
      {CURLOPT_SSH_KNOWNHOSTS,          string_option_setter},
      {CURLOPT_SSH_PRIVATE_KEYFILE,     string_option_setter},
      {CURLOPT_SSH_PUBLIC_KEYFILE,      string_option_setter},
      {CURLOPT_SSLCERT,                 string_option_setter},
      {CURLOPT_SSLCERTTYPE,             string_option_setter},
      {CURLOPT_SSLENGINE,               string_option_setter},
      {CURLOPT_SSLENGINE_DEFAULT,    string_option_setter},
      {CURLOPT_SSLKEY,               string_option_setter},
      {CURLOPT_SSLKEYPASSWD,         string_option_setter},
      {CURLOPT_SSLKEYTYPE,           string_option_setter},
      {CURLOPT_SSL_CIPHER_LIST,      string_option_setter},
      {CURLOPT_URL,                  string_option_setter},
      {CURLOPT_USERAGENT,            string_option_setter},
      {CURLOPT_USERNAME,             string_option_setter},
      {CURLOPT_USERPWD,              string_option_setter},

      {CURLOPT_HTTP200ALIASES,       linked_list_option_setter},
      {CURLOPT_HTTPHEADER,           linked_list_option_setter},
      {CURLOPT_POSTQUOTE,            linked_list_option_setter},
      {CURLOPT_PREQUOTE,             linked_list_option_setter},
      {CURLOPT_QUOTE,                linked_list_option_setter},
      {CURLOPT_MAIL_RCPT,            linked_list_option_setter},

      {CURLOPT_POSTFIELDS,           post_fields_option_setter},

      {CURLOPT_MAX_RECV_SPEED_LARGE, off_option_setter},
      {CURLOPT_MAX_SEND_SPEED_LARGE, off_option_setter},

      {CURLOPT_PUT,                  long_option_setter},

      {CURLOPT_RESOLVE,              linked_list_option_setter},
      {CURLOPT_HTTP_VERSION,         long_option_setter},

      {CURLOPT_SSL_ENABLE_ALPN,      long_option_setter},
      {CURLOPT_SSL_ENABLE_NPN,       long_option_setter},
      {CURLOPT_TCP_KEEPALIVE,        long_option_setter},
      {CURLOPT_TCP_KEEPIDLE,         long_option_setter},
      {CURLOPT_TCP_KEEPINTVL,        long_option_setter},
      {CURLOPT_PRIVATE,              private_option_setter},
    });

  constexpr size_t CURLOPT_OPTION_OFFSET = 200000;
  if (easy_context->check_option_value<CURLOPT_OPTION_OFFSET, curlopt_options.size()>(option, "parameter option", "curl_setopt")) {
    auto curl_option = curlopt_options[option - CURLOPT_OPTION_OFFSET];
    easy_context->error_num = CURLE_OK;
    curl_option.option_setter(easy_context, curl_option.option, value);
  }
  return easy_context->error_num == CURLE_OK;
}

void long_multi_option_setter(MultiContext *multi_context, CURLMoption option, int64_t value) {
  multi_context->set_option_safe(option, static_cast<long>(value));
}

void off_multi_option_setter(MultiContext *multi_context, CURLMoption option, int64_t value) {
  multi_context->set_option_safe(option, static_cast<curl_off_t>(value));
}

} // namespace

void init_curl_lib() noexcept;

curl_easy f$curl_init(const string &url) noexcept {
  init_curl_lib();

  CURL *easy_handle = dl::critical_section_call(curl_easy_init);
  if (unlikely(easy_handle == nullptr)) {
    php_warning("Could not initialize a new curl easy handle");
    return 0;
  }

  const int64_t curl_handler_id = CurlContexts::get()->easy_contexts.count() + 1;
  auto *easy_context = static_cast<EasyContext *>(dl::allocate(sizeof(EasyContext)));
  new(easy_context) EasyContext(easy_handle, curl_handler_id);
  easy_context->set_default_options();

  if (!url.empty() && easy_context->set_option_safe(CURLOPT_URL, url.c_str()) != CURLE_OK) {
    php_warning("Could not set url to a new curl easy handle");
    easy_close(easy_context);
    return 0;
  }

  CurlContexts::get()->easy_contexts.push_back(easy_context);
  return curl_handler_id;
}

void f$curl_reset(curl_easy easy_id) noexcept {
  if (auto *easy_context = get_context<EasyContext>(easy_id)) {
    curl_easy_reset(easy_context->easy_handle);
    easy_context->return_transfer = false;
    easy_context->private_data = false;
    easy_context->cleanup_slists_and_posts();
    easy_context->cleanup_for_next_request();
    easy_context->set_default_options();
  }
}

bool f$curl_setopt(curl_easy easy_id, int64_t option, const mixed &value) noexcept {
  if (auto *easy_context = get_context<EasyContext>(easy_id)) {
    if (curl_setopt(easy_context, option, value)) {
      return true;
    }
    php_warning("Can't set curl option %ld", option);
  }
  return false;
}

bool f$curl_setopt_array(curl_easy easy_id, const array<mixed> &options) noexcept {
  if (auto *easy_context = get_context<EasyContext>(easy_id)) {
    for (auto p = options.begin(); p != options.end(); ++p) {
      if (!curl_setopt(easy_context, p.get_key().to_int(), p.get_value())) {
        php_warning("Can't set curl option \"%s\"", p.get_key().to_string().c_str());
        return false;
      }
    }
    return true;
  }
  return false;
}

mixed f$curl_exec(curl_easy easy_id) noexcept {
  auto *easy_context = get_context<EasyContext>(easy_id);
  if (!easy_context) {
    return false;
  }

  easy_context->cleanup_for_next_request();
  easy_context->error_num = dl::critical_section_call(curl_easy_perform, easy_context->easy_handle);

  if (easy_context->error_num != CURLE_OK && easy_context->error_num != CURLE_PARTIAL_FILE) {
    return false;
  }

  if (easy_context->return_transfer) {
    return easy_context->result;
  }

  return true;
}

mixed f$curl_getinfo(curl_easy easy_id, int64_t option) noexcept {
  auto *easy_context = get_context<EasyContext>(easy_id);
  if (!easy_context) {
    return false;
  }

  if (option == 0) {
    array<mixed> result(array_size(0, 26, false));
    easy_context->add_info_into_array(result, "url", CURLINFO_EFFECTIVE_URL);
    easy_context->add_info_into_array(result, "content_type", CURLINFO_CONTENT_TYPE);
    easy_context->add_info_into_array(result, "http_code", CURLINFO_RESPONSE_CODE);
    easy_context->add_info_into_array(result, "header_size", CURLINFO_HEADER_SIZE);
    easy_context->add_info_into_array(result, "request_size", CURLINFO_REQUEST_SIZE);
    easy_context->add_info_into_array(result, "filetime", CURLINFO_FILETIME);
    easy_context->add_info_into_array(result, "ssl_verify_result", CURLINFO_SSL_VERIFYRESULT);
    easy_context->add_info_into_array(result, "redirect_count", CURLINFO_REDIRECT_COUNT);
    easy_context->add_info_into_array(result, "total_time", CURLINFO_TOTAL_TIME);
    easy_context->add_info_into_array(result, "namelookup_time", CURLINFO_NAMELOOKUP_TIME);
    easy_context->add_info_into_array(result, "connect_time", CURLINFO_CONNECT_TIME);
    easy_context->add_info_into_array(result, "pretransfer_time", CURLINFO_PRETRANSFER_TIME);
    easy_context->add_info_into_array(result, "size_upload", CURLINFO_SIZE_UPLOAD);
    easy_context->add_info_into_array(result, "size_download", CURLINFO_SIZE_DOWNLOAD);
    easy_context->add_info_into_array(result, "speed_download", CURLINFO_SPEED_DOWNLOAD);
    easy_context->add_info_into_array(result, "speed_upload", CURLINFO_SPEED_UPLOAD);
    easy_context->add_info_into_array(result, "download_content_length", CURLINFO_CONTENT_LENGTH_DOWNLOAD);
    easy_context->add_info_into_array(result, "upload_content_length", CURLINFO_CONTENT_LENGTH_UPLOAD);
    easy_context->add_info_into_array(result, "starttransfer_time", CURLINFO_STARTTRANSFER_TIME);
    easy_context->add_info_into_array(result, "redirect_time", CURLINFO_REDIRECT_TIME);
    easy_context->add_info_into_array(result, "redirect_url", CURLINFO_REDIRECT_URL);
    easy_context->add_info_into_array(result, "primary_ip", CURLINFO_PRIMARY_IP);
    easy_context->add_info_into_array(result, "primary_port", CURLINFO_PRIMARY_PORT);
    easy_context->add_info_into_array(result, "local_ip", CURLINFO_LOCAL_IP);
    easy_context->add_info_into_array(result, "local_port", CURLINFO_LOCAL_PORT);
    result.set_value(string{"request_header"}, easy_context->header);
    return result;
  }

  if (option == CURLOPT_INFO_HEADER_OUT) {
    return easy_context->header;
  }

  constexpr static auto curlinfo_options = vk::to_array<CURLINFO>(
    {
      CURLINFO_EFFECTIVE_URL,
      CURLINFO_RESPONSE_CODE,
      CURLINFO_FILETIME,
      CURLINFO_TOTAL_TIME,
      CURLINFO_NAMELOOKUP_TIME,
      CURLINFO_CONNECT_TIME,
      CURLINFO_PRETRANSFER_TIME,
      CURLINFO_STARTTRANSFER_TIME,
      CURLINFO_REDIRECT_COUNT,
      CURLINFO_REDIRECT_TIME,
      CURLINFO_SIZE_UPLOAD,
      CURLINFO_SIZE_DOWNLOAD,
      CURLINFO_SPEED_UPLOAD,
      CURLINFO_HEADER_SIZE,
      CURLINFO_REQUEST_SIZE,
      CURLINFO_SSL_VERIFYRESULT,
      CURLINFO_CONTENT_LENGTH_DOWNLOAD,
      CURLINFO_CONTENT_LENGTH_UPLOAD,
      CURLINFO_CONTENT_TYPE,
      CURLINFO_PRIVATE,
      CURLINFO_SPEED_DOWNLOAD,
      CURLINFO_REDIRECT_URL,
      CURLINFO_PRIMARY_IP,
      CURLINFO_PRIMARY_PORT,
      CURLINFO_LOCAL_IP,
      CURLINFO_LOCAL_PORT,
      CURLINFO_HTTP_CONNECTCODE,
      CURLINFO_HTTPAUTH_AVAIL,
      CURLINFO_PROXYAUTH_AVAIL,
      CURLINFO_OS_ERRNO,
      CURLINFO_NUM_CONNECTS,
      CURLINFO_FTP_ENTRY_PATH,
      CURLINFO_APPCONNECT_TIME,
      CURLINFO_CONDITION_UNMET,
      CURLINFO_RTSP_CLIENT_CSEQ,
      CURLINFO_RTSP_CSEQ_RECV,
      CURLINFO_RTSP_SERVER_CSEQ,
      CURLINFO_RTSP_SESSION_ID,
    });

  constexpr size_t CURLINFO_OPTION_OFFSET = 100000;
  return easy_context->check_option_value<CURLINFO_OPTION_OFFSET, curlinfo_options.size()>(option, "parameter option", "curl_getinfo", false)
         ? easy_context->get_info(curlinfo_options[option - CURLINFO_OPTION_OFFSET])
         : false;
}

string f$curl_error(curl_easy easy_id) noexcept {
  auto *easy_context = get_context<EasyContext>(easy_id);
  return (easy_context && easy_context->error_num != CURLE_OK) ? string{easy_context->error_msg} : string{};
}

int64_t f$curl_errno(curl_easy easy_id) noexcept {
  auto *easy_context = get_context<EasyContext>(easy_id);
  return easy_context ? easy_context->error_num : 0;
}

void f$curl_close(curl_easy easy_id) noexcept {
  if (auto *easy_context = get_context<EasyContext>(easy_id)) {
    CurlContexts::get()->easy_contexts.set_value(easy_id - 1, nullptr);
    easy_close(easy_context);
  }
}

curl_multi f$curl_multi_init() noexcept {
  init_curl_lib();

  CURLM *multi_handle = dl::critical_section_call(curl_multi_init);
  if (unlikely(multi_handle == nullptr)) {
    php_warning("Could not initialize a new curl multi handle");
    return 0;
  }

  auto *multi = static_cast<MultiContext *>(dl::allocate(sizeof(MultiContext)));
  new(multi) MultiContext{multi_handle};

  CurlContexts::get()->multi_contexts.push_back(multi);
  return CurlContexts::get()->multi_contexts.count();
}

Optional<int64_t> f$curl_multi_add_handle(curl_multi multi_id, curl_easy easy_id) noexcept {
  if (auto *multi_context = get_context<MultiContext>(multi_id)) {
    if (auto *easy_context = get_context<EasyContext>(easy_id)) {
      easy_context->cleanup_for_next_request();
      multi_context->error_num = dl::critical_section_call(curl_multi_add_handle, multi_context->multi_handle, easy_context->easy_handle);
      return multi_context->error_num;
    }
  }
  return false;
}

Optional<string> f$curl_multi_getcontent(curl_easy easy_id) noexcept {
  if (auto *easy_context = get_context<EasyContext>(easy_id)) {
    return easy_context->return_transfer ? easy_context->result : Optional<string>{};
  }
  return false;
}

bool f$curl_multi_setopt(curl_multi multi_id, int64_t option, int64_t value) noexcept {
  auto *multi_context = get_context<MultiContext>(multi_id);
  if (!multi_context) {
    return false;
  }

  struct MultiOptionHandler {
    CURLMoption option;
    void (*option_setter)(MultiContext *multi_context, CURLMoption option, int64_t value);
  };
  constexpr static auto multi_options = vk::to_array<MultiOptionHandler>(
    {
      {CURLMOPT_PIPELINING,                  long_multi_option_setter},
      {CURLMOPT_MAXCONNECTS,                 long_multi_option_setter},

      {CURLMOPT_CHUNK_LENGTH_PENALTY_SIZE,   off_multi_option_setter},
      {CURLMOPT_CONTENT_LENGTH_PENALTY_SIZE, off_multi_option_setter},

      {CURLMOPT_MAX_HOST_CONNECTIONS,        long_multi_option_setter},
      {CURLMOPT_MAX_PIPELINE_LENGTH,         long_multi_option_setter},
      {CURLMOPT_MAX_TOTAL_CONNECTIONS,       long_multi_option_setter}
    });

  constexpr size_t CURLMULTI_OPTION_OFFSET = 1000;
  if (multi_context->check_option_value<CURLMULTI_OPTION_OFFSET, multi_options.size()>(option, "parameter option", "curl_multi_setopt")) {
    auto multi_option = multi_options[option - CURLMULTI_OPTION_OFFSET];
    multi_context->error_num = CURLM_OK;
    multi_option.option_setter(multi_context, multi_option.option, value);
  }
  return multi_context->error_num == CURLM_OK;
}

Optional<int64_t> f$curl_multi_exec(curl_multi multi_id, int64_t &still_running) noexcept {
  if (auto *multi_context = get_context<MultiContext>(multi_id)) {
    int still_running_int = 0;
    multi_context->error_num = dl::critical_section_call(curl_multi_perform, multi_context->multi_handle, &still_running_int);
    still_running = still_running_int;
    return multi_context->error_num;
  }
  return false;
}

Optional<int64_t> f$curl_multi_select(curl_multi multi_id, double timeout) noexcept {
  if (auto *multi_context = get_context<MultiContext>(multi_id)) {
    int numfds = 0;
    multi_context->error_num = dl::critical_section_call(curl_multi_wait, multi_context->multi_handle, nullptr, 0,
                                                         static_cast<int>(timeout * 1000.0), &numfds);
    if (multi_context->error_num != CURLM_OK) {
      return -1;
    }
    return int64_t{numfds};
  }
  return false;
}

int64_t curl_multi_info_read_msgs_in_queue_stub = 0;
Optional<array<int64_t>> f$curl_multi_info_read(curl_multi multi_id, int64_t &msgs_in_queue) {
  if (auto *multi_context = get_context<MultiContext>(multi_id)) {
    int msgs_in_queue_int = 0;
    CURLMsg *msg = dl::critical_section_call(curl_multi_info_read, multi_context->multi_handle, &msgs_in_queue_int);
    msgs_in_queue = msgs_in_queue_int;
    if (msg) {
      array<int64_t> result{array_size{0, 3, false}};
      result.set_value(string{"msg"}, static_cast<int64_t>(msg->msg));
      result.set_value(string{"result"}, static_cast<int64_t>(msg->data.result));

      void *id_as_ptr = nullptr;
      dl::critical_section_call([&] { curl_easy_getinfo (msg->easy_handle, CURLINFO_PRIVATE, &id_as_ptr); });
      const auto curl_handler_id = static_cast<int64_t>(reinterpret_cast<size_t>(id_as_ptr));
      const auto *easy_handle = CurlContexts::get()->easy_contexts.find_value(curl_handler_id - 1);
      if (easy_handle && (*easy_handle)->easy_handle == msg->easy_handle) {
        (*easy_handle)->error_num = msg->data.result;
        result.set_value(string{"handle"}, curl_handler_id);
      }
      return result;
    }
  }
  return false;
}

Optional<int64_t> f$curl_multi_remove_handle(curl_multi multi_id, curl_easy easy_id) noexcept {
  if (auto *multi_context = get_context<MultiContext>(multi_id)) {
    if (auto *easy_context = get_context<EasyContext>(easy_id)) {
      multi_context->error_num = dl::critical_section_call(curl_multi_remove_handle, multi_context->multi_handle, easy_context->easy_handle);
      return multi_context->error_num;
    }
  }
  return false;
}

Optional<int64_t> f$curl_multi_errno(curl_multi multi_id) noexcept {
  auto *multi_context = get_context<MultiContext>(multi_id);
  return multi_context ? multi_context->error_num : false;
}

void f$curl_multi_close(curl_multi multi_id) noexcept {
  if (auto *multi_context = get_context<MultiContext>(multi_id)) {
    CurlContexts::get()->multi_contexts.set_value(multi_id - 1, nullptr);
    multi_close(multi_context);
  }
}

Optional<string> f$curl_multi_strerror(int64_t error_num) noexcept {
  if (error_num == BAD_CURL_OPTION) {
    return string{"Bad curl option"};
  }
  const char *err_str = dl::critical_section_call(curl_multi_strerror, static_cast<CURLMcode>(error_num));
  return err_str ? string{err_str} : Optional<string>{};
}

void init_curl_lib() noexcept {
  if (dl::query_num != CurlContexts::get().get_query_tag()) {
    const CURLcode result = dl::critical_section_call([] {
      return curl_global_init_mem(
        CURL_GLOBAL_ALL,
        dl::script_allocator_malloc,
        dl::script_allocator_free,
        dl::script_allocator_realloc,
        dl::script_allocator_strdup,
        dl::script_allocator_calloc);
    });

    if (result != CURLE_OK) {
      php_critical_error ("can't initialize curl");
    }

    CurlContexts::get().init(dl::query_num);
  }
}

void free_curl_lib() noexcept {
  dl::CriticalSectionGuard critical_section;
  if (dl::query_num == CurlContexts::get().get_query_tag()) {
    for (auto it = CurlContexts::get()->easy_contexts.cbegin(); it != CurlContexts::get()->easy_contexts.cend(); ++it) {
      if (auto easy_context = it.get_value()) {
        easy_close(easy_context);
      }
    }

    for (auto it = CurlContexts::get()->multi_contexts.cbegin(); it != CurlContexts::get()->multi_contexts.cend(); ++it) {
      if (auto multi_context = it.get_value()) {
        multi_close(multi_context);
      }
    }

    curl_global_cleanup();
    CurlContexts::get().hard_reset();
    // curl_global_cleanup() from libcurl de-initializes openssl completely
    // TODO: understand how to live without curl_global_cleanup() and remove this kludge from here
    reinit_openssl_lib_hack();
  }
}
