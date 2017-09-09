#include "runtime/curl.h"

#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <curl/easy.h>

#include "runtime/integer_types.h"
#include "runtime/interface.h"

//TODO
//curl-config --static-libs
//-Wl,-z,relro -Wl,--as-needed -lidn -lssh2 -lssl -llber -lldap -lgssapi_krb5 -lkrb5 -lk5crypto -lcom_err -lrtmp -lgnutls
//-lidn -lssh2 -lssl -llber -lldap -lgssapi_krb5 

#if LIBCURL_VERSION_NUM < 0x071500
#  error Outdated libcurl
#endif

void curl_init_static (void);


const int LAST_ERROR_NO = CURL_LAST + CURL_FORMADD_LAST;

class curl_handler {
public:
  CURL *cp;

  int id;

  string header;
  string result;

  array <curl_slist *> slists_to_free;
  array <curl_httppost *> httpposts_to_free;

  var (*header_function) (var curl_id, var str);

  bool in_callback;
  bool return_transfer;

  char error[CURL_ERROR_SIZE + 1];
  int error_no;
};

static char curl_handlers_storage[sizeof (array <curl_handler *>)];
static array <curl_handler *> *curl_handlers = (array <curl_handler *> *)curl_handlers_storage;
static long long curl_last_query_num = -1;

static curl_handler *get_curl_handler (curl id) {
  if (dl::query_num != curl_last_query_num) {
    return NULL;
  }

  return curl_handlers->get_value (id - 1);
}

static void curl_close (curl_handler *ch);

static size_t curl_write (char *data, size_t size, size_t nmemb, void *userdata) {
  dl::leave_critical_section();//this is a callback called from curl_easy_perform

  curl_handler *ch = static_cast <curl_handler *> (userdata);
  php_assert (CURL_MAX_WRITE_SIZE <= (1 << 30));
  int length = static_cast <int> (size * nmemb);

  if (ch->return_transfer) {
    ch->result.reserve_at_least (ch->result.size() + length);
    ch->result.append_unsafe (data, length);
  } else {
    print (data, length);
  }

  dl::enter_critical_section();//OK
  return size * nmemb;
}


static size_t curl_write_header (char *data, size_t size, size_t nmemb, void *userdata) {
  dl::leave_critical_section();//this is a callback called from curl_easy_perform

  curl_handler *ch = static_cast <curl_handler *> (userdata);
  php_assert (CURL_MAX_WRITE_SIZE <= (1 << 30));
  int length = static_cast <int> (size * nmemb);

  size_t result = size * nmemb;
  if (ch->header_function != NULL) {
    ch->in_callback = true;
    result = (size_t)f$intval (ch->header_function (ch->id, string (data, length)));
    ch->in_callback = false;
  }

  dl::enter_critical_section();//OK
  return result;
}

static int curl_info_header_out (CURL *cp __attribute__((unused)), curl_infotype type, char *buf, size_t buf_len, void *userdata) {
  dl::leave_critical_section();//this is a callback called from curl_easy_perform

  curl_handler *ch = static_cast <curl_handler *> (userdata);

  if (type == CURLINFO_HEADER_OUT) {
    ch->header = string (buf, buf_len);
  }

  dl::enter_critical_section();//OK
  return 0;
}

curl f$curl_init (const string &url) {
  curl_init_static();

  dl::enter_critical_section();//OK
  CURL *cp = curl_easy_init();
  dl::leave_critical_section();
  if (cp == NULL) {
    php_warning ("Could not initialize a new cURL handle");
    return 0;
  }

  curl_handler *ch = static_cast <curl_handler *> (dl::allocate (sizeof (curl_handler)));
  new (ch) curl_handler();

  ch->cp = cp;
  ch->header_function = NULL;
  ch->in_callback = false;
  ch->return_transfer = false;

  dl::enter_critical_section();//OK
  php_assert (curl_easy_setopt (cp, CURLOPT_NOPROGRESS, 1L) == CURLE_OK);
  php_assert (curl_easy_setopt (cp, CURLOPT_VERBOSE, 0L) == CURLE_OK);
  php_assert (curl_easy_setopt (cp, CURLOPT_ERRORBUFFER, ch->error) == CURLE_OK);
  php_assert (curl_easy_setopt (cp, CURLOPT_WRITEFUNCTION, curl_write) == CURLE_OK);
  php_assert (curl_easy_setopt (cp, CURLOPT_WRITEDATA, static_cast <void *> (ch)) == CURLE_OK);
  php_assert (curl_easy_setopt (cp, CURLOPT_HEADERFUNCTION, curl_write_header) == CURLE_OK);
  php_assert (curl_easy_setopt (cp, CURLOPT_WRITEHEADER, static_cast <void *> (ch)) == CURLE_OK);
  php_assert (curl_easy_setopt (cp, CURLOPT_DNS_USE_GLOBAL_CACHE, 1L) == CURLE_OK);
  php_assert (curl_easy_setopt (cp, CURLOPT_DNS_CACHE_TIMEOUT, 120L) == CURLE_OK);
  php_assert (curl_easy_setopt (cp, CURLOPT_MAXREDIRS, 20L) == CURLE_OK);

  php_assert (curl_easy_setopt (cp, CURLOPT_NOSIGNAL, 1L) == CURLE_OK);

  php_assert (curl_easy_setopt (cp, CURLOPT_PROTOCOLS, (long)(CURLPROTO_ALL & ~(CURLPROTO_FILE | CURLPROTO_SCP))) == CURLE_OK);//Always disabled FILE and SCP

  if (url.size()) {
    if (curl_easy_setopt (cp, CURLOPT_URL, url.c_str()) != CURLE_OK) {
      dl::leave_critical_section();
      php_warning ("Could not set url to a new cURL handle");
      
      curl_close (ch);
      return 0;
    }
  }
  dl::leave_critical_section();

  curl_handlers->push_back (ch);
  ch->id = curl_handlers->count();
  return ch->id;
}

const int CURLOPT_RETURNTRANSFER = 1234567;
const int CURLOPT_INFO_HEADER_OUT = 7654321;

static bool curl_setopt (curl_handler *ch, int option, const var &value) {
  if (option == CURLOPT_INFO_HEADER_OUT) {
    dl::enter_critical_section();//OK
    if (f$longval (value).l == 1ll) {
      php_assert (curl_easy_setopt (ch->cp, CURLOPT_DEBUGFUNCTION, curl_info_header_out) == CURLE_OK);
      php_assert (curl_easy_setopt (ch->cp, CURLOPT_DEBUGDATA, (void *)ch) == CURLE_OK);
      php_assert (curl_easy_setopt (ch->cp, CURLOPT_VERBOSE, 1) == CURLE_OK);
    } else {
      php_assert (curl_easy_setopt (ch->cp, CURLOPT_DEBUGFUNCTION, NULL) == CURLE_OK);
      php_assert (curl_easy_setopt (ch->cp, CURLOPT_DEBUGDATA, NULL) == CURLE_OK);
      php_assert (curl_easy_setopt (ch->cp, CURLOPT_VERBOSE, 0) == CURLE_OK);
    }
    dl::leave_critical_section();

    return true;
  }

  if (option == CURLOPT_RETURNTRANSFER) {
    ch->return_transfer = (f$longval (value).l == 1ll);
    return true;
  }

  const int CURLOPT_FIRST_OPTION_NUMBER = 200000;
  const static CURLoption curlopt_options[] = {
    CURLOPT_ADDRESS_SCOPE,
    CURLOPT_APPEND,
    CURLOPT_AUTOREFERER,
    CURLOPT_BUFFERSIZE,
    CURLOPT_CONNECT_ONLY,
    CURLOPT_CONNECTTIMEOUT,
    CURLOPT_CONNECTTIMEOUT_MS,
    CURLOPT_COOKIESESSION,
    CURLOPT_CRLF,
    CURLOPT_DIRLISTONLY,
    CURLOPT_DNS_CACHE_TIMEOUT,
    CURLOPT_FAILONERROR,
    CURLOPT_FILETIME,
    CURLOPT_FOLLOWLOCATION,
    CURLOPT_FORBID_REUSE,
    CURLOPT_FRESH_CONNECT,
    CURLOPT_FTP_CREATE_MISSING_DIRS,
    CURLOPT_FTP_RESPONSE_TIMEOUT,
    CURLOPT_FTP_SKIP_PASV_IP,
    CURLOPT_FTP_USE_EPRT,
    CURLOPT_FTP_USE_EPSV,
    CURLOPT_FTP_USE_PRET,
    CURLOPT_HEADER,
    CURLOPT_HTTP_CONTENT_DECODING,
    CURLOPT_HTTP_TRANSFER_DECODING,
    CURLOPT_HTTPGET,
    CURLOPT_HTTPPROXYTUNNEL,
    CURLOPT_IGNORE_CONTENT_LENGTH,
    CURLOPT_INFILESIZE,
    CURLOPT_LOW_SPEED_LIMIT,
    CURLOPT_LOW_SPEED_TIME,
    CURLOPT_MAXCONNECTS,
    CURLOPT_MAXFILESIZE,
    CURLOPT_MAXREDIRS,
    CURLOPT_NETRC,
    CURLOPT_NEW_DIRECTORY_PERMS,
    CURLOPT_NEW_FILE_PERMS,
    CURLOPT_NOBODY,
    CURLOPT_PORT,
    CURLOPT_POST,
    CURLOPT_PROXY_TRANSFER_MODE,
    CURLOPT_PROXYPORT,
    CURLOPT_RESUME_FROM,
    CURLOPT_SOCKS5_GSSAPI_NEC,
    CURLOPT_SSL_SESSIONID_CACHE,
    CURLOPT_SSL_VERIFYHOST,
    CURLOPT_SSL_VERIFYPEER,
    CURLOPT_TCP_NODELAY,
    CURLOPT_TFTP_BLKSIZE,
    CURLOPT_TIMEOUT,
    CURLOPT_TIMEOUT_MS,
    CURLOPT_TRANSFERTEXT,
    CURLOPT_UNRESTRICTED_AUTH,
    CURLOPT_UPLOAD,
    CURLOPT_VERBOSE,
    CURLOPT_WILDCARDMATCH,
    CURLOPT_PROXYTYPE,
    CURLOPT_SSLVERSION,
    CURLOPT_HTTPAUTH,
    CURLOPT_PROXYAUTH,
    CURLOPT_IPRESOLVE,
    CURLOPT_FTPSSLAUTH,
    CURLOPT_FTP_FILEMETHOD,
    CURLOPT_CAINFO,
    CURLOPT_CAPATH,
    CURLOPT_COOKIE,
    CURLOPT_COOKIEFILE,
    CURLOPT_COOKIEJAR,
    CURLOPT_COOKIELIST,
    CURLOPT_CRLFILE,
    CURLOPT_CUSTOMREQUEST,
    CURLOPT_EGDSOCKET,
    CURLOPT_FTP_ACCOUNT,
    CURLOPT_FTP_ALTERNATIVE_TO_USER,
    CURLOPT_FTPPORT,
    CURLOPT_INTERFACE,
    CURLOPT_ISSUERCERT,
    CURLOPT_KRBLEVEL,
    CURLOPT_MAIL_FROM,
    CURLOPT_NETRC_FILE,
    CURLOPT_NOPROXY,
    CURLOPT_PASSWORD,
    CURLOPT_PROXY,
    CURLOPT_PROXYPASSWORD,
    CURLOPT_PROXYUSERNAME,
    CURLOPT_PROXYUSERPWD,
    CURLOPT_RANDOM_FILE,
    CURLOPT_RANGE,
    CURLOPT_REFERER,
    CURLOPT_RTSP_SESSION_ID,
    CURLOPT_RTSP_STREAM_URI,
    CURLOPT_RTSP_TRANSPORT,
    CURLOPT_SOCKS5_GSSAPI_SERVICE,
    CURLOPT_SSH_HOST_PUBLIC_KEY_MD5,
    CURLOPT_SSH_KNOWNHOSTS,
    CURLOPT_SSH_PRIVATE_KEYFILE,
    CURLOPT_SSH_PUBLIC_KEYFILE,
    CURLOPT_SSLCERT,
    CURLOPT_SSLCERTTYPE,
    CURLOPT_SSLENGINE,
    CURLOPT_SSLENGINE_DEFAULT,
    CURLOPT_SSLKEY,
    CURLOPT_SSLKEYPASSWD,
    CURLOPT_SSLKEYTYPE,
    CURLOPT_SSL_CIPHER_LIST,
    CURLOPT_URL,
    CURLOPT_USERAGENT,
    CURLOPT_USERNAME,
    CURLOPT_USERPWD,
    CURLOPT_HTTP200ALIASES,
    CURLOPT_HTTPHEADER,
    CURLOPT_POSTQUOTE,
    CURLOPT_PREQUOTE,
    CURLOPT_QUOTE,
    CURLOPT_MAIL_RCPT,
    CURLOPT_POSTFIELDS,
    CURLOPT_MAX_RECV_SPEED_LARGE,
    CURLOPT_MAX_SEND_SPEED_LARGE,
    CURLOPT_PUT};
  const int CURLOPT_OPTIONS_COUNT = sizeof (curlopt_options) / sizeof (curlopt_options[0]);
  
  option -= CURLOPT_FIRST_OPTION_NUMBER;
  if ((unsigned int)option > (unsigned int)CURLOPT_OPTIONS_COUNT) {
    php_warning ("Wrong parameter option %d in function curl_setopt", option + CURLOPT_FIRST_OPTION_NUMBER);
    ch->error_no = LAST_ERROR_NO;
    snprintf (ch->error, CURL_ERROR_SIZE, "Wrong parameter option %d", option + CURLOPT_FIRST_OPTION_NUMBER);
    return false;
  }
  CURLoption curl_option = curlopt_options[option];

  ch->error_no = CURLE_OK;

  switch (curl_option) {
    case CURLOPT_ADDRESS_SCOPE:
    case CURLOPT_APPEND:
    case CURLOPT_AUTOREFERER:
    case CURLOPT_BUFFERSIZE:
    case CURLOPT_CONNECT_ONLY:
    case CURLOPT_CONNECTTIMEOUT:
    case CURLOPT_CONNECTTIMEOUT_MS:
    case CURLOPT_COOKIESESSION:
    case CURLOPT_CRLF:
    case CURLOPT_DIRLISTONLY:
    case CURLOPT_DNS_CACHE_TIMEOUT:
    case CURLOPT_FAILONERROR:
    case CURLOPT_FILETIME:
    case CURLOPT_FOLLOWLOCATION:
    case CURLOPT_FORBID_REUSE:
    case CURLOPT_FRESH_CONNECT:
    case CURLOPT_FTP_CREATE_MISSING_DIRS:
    case CURLOPT_FTP_RESPONSE_TIMEOUT:
    case CURLOPT_FTP_SKIP_PASV_IP:
    case CURLOPT_FTP_USE_EPRT:
    case CURLOPT_FTP_USE_EPSV:
    case CURLOPT_FTP_USE_PRET:
    case CURLOPT_HEADER:
    case CURLOPT_HTTP_CONTENT_DECODING:
    case CURLOPT_HTTP_TRANSFER_DECODING:
    case CURLOPT_HTTPGET:
    case CURLOPT_HTTPPROXYTUNNEL:
    case CURLOPT_IGNORE_CONTENT_LENGTH:
    case CURLOPT_INFILESIZE:
    case CURLOPT_LOW_SPEED_LIMIT:
    case CURLOPT_LOW_SPEED_TIME:
    case CURLOPT_MAXCONNECTS:
    case CURLOPT_MAXFILESIZE:
    case CURLOPT_MAXREDIRS:
    case CURLOPT_NETRC:
    case CURLOPT_NEW_DIRECTORY_PERMS:
    case CURLOPT_NEW_FILE_PERMS:
    case CURLOPT_NOBODY:
    case CURLOPT_PORT:
    case CURLOPT_POST:
    case CURLOPT_PROXY_TRANSFER_MODE:
    case CURLOPT_PROXYPORT:
    case CURLOPT_RESUME_FROM:
    case CURLOPT_SOCKS5_GSSAPI_NEC:
    case CURLOPT_SSL_SESSIONID_CACHE:
    case CURLOPT_SSL_VERIFYHOST:
    case CURLOPT_SSL_VERIFYPEER:
    case CURLOPT_TCP_NODELAY:
    case CURLOPT_TFTP_BLKSIZE:
    case CURLOPT_TIMEOUT:
    case CURLOPT_TIMEOUT_MS:
    case CURLOPT_TRANSFERTEXT:
    case CURLOPT_UNRESTRICTED_AUTH:
    case CURLOPT_UPLOAD:
    case CURLOPT_VERBOSE:
    case CURLOPT_WILDCARDMATCH:
    case CURLOPT_PUT: {
      long val = static_cast <long> (f$longval (value).l);
      dl::enter_critical_section();//OK
      ch->error_no = curl_easy_setopt (ch->cp, curl_option, val);
      dl::leave_critical_section();
      break;
    }
    case CURLOPT_PROXYTYPE: {
      const int FIRST_OPTION_NUMBER = 400000;
      const static long options[] = {
        CURLPROXY_HTTP,
        CURLPROXY_HTTP_1_0,
        CURLPROXY_SOCKS4,
        CURLPROXY_SOCKS5,
        CURLPROXY_SOCKS4A,
        CURLPROXY_SOCKS5_HOSTNAME
      };
      const int OPTIONS_COUNT = sizeof (options) / sizeof (options[0]);

      long val = static_cast <long> (f$longval (value).l);
      if ((unsigned int)(val - FIRST_OPTION_NUMBER) > (unsigned int)OPTIONS_COUNT) {
        php_warning ("Wrong parameter value %ld in function curl_setopt", val);
        ch->error_no = LAST_ERROR_NO;
        snprintf (ch->error, CURL_ERROR_SIZE, "Wrong parameter value %ld", val);
        return false;
      }
      val = options[val - FIRST_OPTION_NUMBER];
      dl::enter_critical_section();//OK
      ch->error_no = curl_easy_setopt (ch->cp, curl_option, val);
      dl::leave_critical_section();
      break;
    }
    case CURLOPT_SSLVERSION: {
      const int FIRST_OPTION_NUMBER = 0;
      const static long options[] = {
        CURL_SSLVERSION_DEFAULT,
        CURL_SSLVERSION_TLSv1,
        CURL_SSLVERSION_SSLv2,
        CURL_SSLVERSION_SSLv3
      };
      const int OPTIONS_COUNT = sizeof (options) / sizeof (options[0]);

      long val = static_cast <long> (f$longval (value).l);
      if ((unsigned int)(val - FIRST_OPTION_NUMBER) > (unsigned int)OPTIONS_COUNT) {
        php_warning ("Wrong parameter value %ld in function curl_setopt", val);
        ch->error_no = LAST_ERROR_NO;
        snprintf (ch->error, CURL_ERROR_SIZE, "Wrong parameter value %ld", val);
        return false;
      }
      val = options[val - FIRST_OPTION_NUMBER];
      dl::enter_critical_section();//OK
      ch->error_no = curl_easy_setopt (ch->cp, curl_option, val);
      dl::leave_critical_section();
      break;
    }
    case CURLOPT_HTTPAUTH:
    case CURLOPT_PROXYAUTH: {
      const int FIRST_OPTION_NUMBER = 600000;

      long val = static_cast <long> (f$longval (value).l);
      if ((unsigned int)(val - FIRST_OPTION_NUMBER) >= (unsigned int)(1 << 4)) {
        php_warning ("Wrong parameter value %ld in function curl_setopt", val);
        ch->error_no = LAST_ERROR_NO;
        snprintf (ch->error, CURL_ERROR_SIZE, "Wrong parameter value %ld", val);
        return false;
      }
      val -= FIRST_OPTION_NUMBER;
      val = (val & 1) * CURLAUTH_BASIC +
          ((val >> 1) & 1) * CURLAUTH_DIGEST +
          ((val >> 2) & 1) * CURLAUTH_GSSNEGOTIATE +
          ((val >> 3) & 1) * CURLAUTH_NTLM;
      dl::enter_critical_section();//OK
      ch->error_no = curl_easy_setopt (ch->cp, curl_option, val);
      dl::leave_critical_section();
      break;
    }
    case CURLOPT_IPRESOLVE: {
      const int FIRST_OPTION_NUMBER = 700000;
      const static long options[] = {
        CURL_IPRESOLVE_WHATEVER,
        CURL_IPRESOLVE_V4,
        CURL_IPRESOLVE_V6
      };
      const int OPTIONS_COUNT = sizeof (options) / sizeof (options[0]);

      long val = static_cast <long> (f$longval (value).l);
      if ((unsigned int)(val - FIRST_OPTION_NUMBER) > (unsigned int)OPTIONS_COUNT) {
        php_warning ("Wrong parameter value %ld in function curl_setopt", val);
        ch->error_no = LAST_ERROR_NO;
        snprintf (ch->error, CURL_ERROR_SIZE, "Wrong parameter value %ld", val);
        return false;
      }
      val = options[val - FIRST_OPTION_NUMBER];
      dl::enter_critical_section();//OK
      ch->error_no = curl_easy_setopt (ch->cp, curl_option, val);
      dl::leave_critical_section();
      break;
    }
    case CURLOPT_FTPSSLAUTH: {
      const int FIRST_OPTION_NUMBER = 800000;
      const static long options[] = {
        CURLFTPAUTH_DEFAULT,
        CURLFTPAUTH_SSL,
        CURLFTPAUTH_TLS
      };
      const int OPTIONS_COUNT = sizeof (options) / sizeof (options[0]);

      long val = static_cast <long> (f$longval (value).l);
      if ((unsigned int)(val - FIRST_OPTION_NUMBER) > (unsigned int)OPTIONS_COUNT) {
        php_warning ("Wrong parameter value %ld in function curl_setopt", val);
        ch->error_no = LAST_ERROR_NO;
        snprintf (ch->error, CURL_ERROR_SIZE, "Wrong parameter value %ld", val);
        return false;
      }
      val = options[val - FIRST_OPTION_NUMBER];
      dl::enter_critical_section();//OK
      ch->error_no = curl_easy_setopt (ch->cp, curl_option, val);
      dl::leave_critical_section();
      break;
    }
    case CURLOPT_FTP_FILEMETHOD: {
      const int FIRST_OPTION_NUMBER = 900000;
      const static long options[] = {
        CURLFTPMETHOD_MULTICWD,
        CURLFTPMETHOD_NOCWD,
        CURLFTPMETHOD_SINGLECWD
      };
      const int OPTIONS_COUNT = sizeof (options) / sizeof (options[0]);

      long val = static_cast <long> (f$longval (value).l);
      if ((unsigned int)(val - FIRST_OPTION_NUMBER) > (unsigned int)OPTIONS_COUNT) {
        php_warning ("Wrong parameter value %ld in function curl_setopt", val);
        ch->error_no = LAST_ERROR_NO;
        snprintf (ch->error, CURL_ERROR_SIZE, "Wrong parameter value %ld", val);
        return false;
      }
      val = options[val - FIRST_OPTION_NUMBER];
      dl::enter_critical_section();//OK
      ch->error_no = curl_easy_setopt (ch->cp, curl_option, val);
      dl::leave_critical_section();
      break;
    }

    /* String options */
    case CURLOPT_CAINFO:
    case CURLOPT_CAPATH:
    case CURLOPT_COOKIE:
    case CURLOPT_COOKIEFILE:
    case CURLOPT_COOKIEJAR:
    case CURLOPT_COOKIELIST:
    case CURLOPT_CRLFILE:
    case CURLOPT_CUSTOMREQUEST:
    case CURLOPT_EGDSOCKET:
    case CURLOPT_FTP_ACCOUNT:
    case CURLOPT_FTP_ALTERNATIVE_TO_USER:
    case CURLOPT_FTPPORT:
    case CURLOPT_INTERFACE:
    case CURLOPT_ISSUERCERT:
    case CURLOPT_KRBLEVEL:
    case CURLOPT_MAIL_FROM:
    case CURLOPT_NETRC_FILE:
    case CURLOPT_NOPROXY:
    case CURLOPT_PASSWORD:
    case CURLOPT_PROXY:
    case CURLOPT_PROXYPASSWORD:
    case CURLOPT_PROXYUSERNAME:
    case CURLOPT_PROXYUSERPWD:
    case CURLOPT_RANDOM_FILE:
    case CURLOPT_RANGE:
    case CURLOPT_REFERER:
    case CURLOPT_RTSP_SESSION_ID:
    case CURLOPT_RTSP_STREAM_URI:
    case CURLOPT_RTSP_TRANSPORT:
    case CURLOPT_SOCKS5_GSSAPI_SERVICE:
    case CURLOPT_SSH_HOST_PUBLIC_KEY_MD5:
    case CURLOPT_SSH_KNOWNHOSTS:
    case CURLOPT_SSH_PRIVATE_KEYFILE:
    case CURLOPT_SSH_PUBLIC_KEYFILE:
    case CURLOPT_SSLCERT:
    case CURLOPT_SSLCERTTYPE:
    case CURLOPT_SSLENGINE:
    case CURLOPT_SSLENGINE_DEFAULT:
    case CURLOPT_SSLKEY:
    case CURLOPT_SSLKEYPASSWD:
    case CURLOPT_SSLKEYTYPE:
    case CURLOPT_SSL_CIPHER_LIST:
    case CURLOPT_URL:
    case CURLOPT_USERAGENT:
    case CURLOPT_USERNAME:
    case CURLOPT_USERPWD:
      dl::enter_critical_section();//OK
      ch->error_no = curl_easy_setopt (ch->cp, curl_option, value.to_string().c_str());
      dl::leave_critical_section();
      break;

    /* Curl linked list options */
    case CURLOPT_HTTP200ALIASES:
    case CURLOPT_HTTPHEADER:
    case CURLOPT_POSTQUOTE:
    case CURLOPT_PREQUOTE:
    case CURLOPT_QUOTE:
    case CURLOPT_MAIL_RCPT: {
      if (!value.is_array()) {
        php_warning ("Value must be an array in function curl_setopt");
        ch->error_no = LAST_ERROR_NO;
        snprintf (ch->error, CURL_ERROR_SIZE, "Value must be an array, type of value is %s", value.get_type_c_str());
        return false;
      }

      const array <var> &v = value.to_array();
      curl_slist *slist = NULL;

      for (array <var>::const_iterator p = v.begin(); p != v.end(); ++p) {
        dl::enter_critical_section();//OK
        slist = curl_slist_append (slist, f$strval (p.get_value()).c_str());
        dl::leave_critical_section();

        if (!slist) {
          php_warning ("Can't build curl_slist. How is it possible?");
          ch->error_no = LAST_ERROR_NO;
          snprintf (ch->error, CURL_ERROR_SIZE, "Can't build curl_slist. How is it possible?");
          return false;
        }
      }

      if (slist) {
        ch->slists_to_free.push_back (slist);
        dl::enter_critical_section();//OK
        ch->error_no = curl_easy_setopt (ch->cp, curl_option, slist);
        dl::leave_critical_section();
      }

      break;
    }

    case CURLOPT_POSTFIELDS:
      if (value.is_array()) {
        const array <var> &postfields = value.to_array();

        struct curl_httppost *first = NULL;
        struct curl_httppost *last = NULL;

        for (array <var>::const_iterator p = postfields.begin(); p != postfields.end(); ++p) {
          const string key = f$strval (p.get_key());
          const string v = f$strval (p.get_value());

          if (v[0] == '@') {
            php_critical_error ("files posting is not supported in curl_setopt with CURLOPT_POSTFIELDS\n");
          }

          dl::enter_critical_section();//OK
          ch->error_no = curl_formadd (&first, &last,
              CURLFORM_COPYNAME, key.c_str(),
              CURLFORM_NAMELENGTH, static_cast <long> (key.size()),
              CURLFORM_COPYCONTENTS, v.c_str(),
              CURLFORM_CONTENTSLENGTH, static_cast <long> (v.size()),
              CURLFORM_END);
          dl::leave_critical_section();

          if (ch->error_no != 0) {
            ch->error_no += CURL_LAST;
            break;
          }
        }

        if (ch->error_no != CURLE_OK) {
          dl::enter_critical_section();//OK
          curl_formfree (first);
          dl::leave_critical_section();

          php_warning ("Call to curl_formadd has failed");
          ch->error_no = LAST_ERROR_NO;
          snprintf (ch->error, CURL_ERROR_SIZE, "Call to curl_formadd has failed");
          return false;
        }

        if (first != NULL) {
          ch->httpposts_to_free.push_back (first);
          dl::enter_critical_section();//OK
          ch->error_no = curl_easy_setopt (ch->cp, CURLOPT_HTTPPOST, first);
          dl::leave_critical_section();
        }
      } else {
        string post = value.to_string();
        dl::enter_critical_section();//OK
        ch->error_no = curl_easy_setopt (ch->cp, CURLOPT_POSTFIELDSIZE, post.size());
        php_assert (ch->error_no == CURLE_OK);
        ch->error_no = curl_easy_setopt (ch->cp, CURLOPT_COPYPOSTFIELDS, post.c_str());
        dl::leave_critical_section();
      }
      break;

    case CURLOPT_MAX_RECV_SPEED_LARGE:
    case CURLOPT_MAX_SEND_SPEED_LARGE: {
      curl_off_t v = static_cast <curl_off_t> (f$longval (value).l);
      dl::enter_critical_section();//OK
      ch->error_no = curl_easy_setopt (ch->cp, curl_option, v);
      dl::leave_critical_section();
      break;
    }
    default:
      php_assert (0);
  }

  return (ch->error_no == CURLE_OK);
}

bool f$curl_set_header_function (curl id, var (*header_function) (var curl_id, var string)) {
  curl_handler *ch = get_curl_handler (id);
  if (ch == NULL) {
    php_warning ("Wrong curl handler specified");
    return false;
  }

  if (ch->in_callback) {
    php_warning ("Can't call curl_set_header_function from a callback");
    return false;
  }

  ch->header_function = header_function;

  return true;
}

bool f$curl_setopt (curl id, int option, const var &value) {
  curl_handler *ch = get_curl_handler (id);
  if (ch == NULL) {
    php_warning ("Wrong curl handler specified");
    return false;
  }

  if (ch->in_callback) {
    php_warning ("Can't call curl_setopt from a callback");
    return false;
  }

  if (!curl_setopt (ch, option, value)) {
    php_warning ("Can't set curl option %d", option);
    return false;
  }

  return true;
}

bool f$curl_setopt_array (curl id, const array <var> &options) {
  curl_handler *ch = get_curl_handler (id);
  if (ch == NULL) {
    php_warning ("Wrong curl handler specified");
    return false;
  }

  if (ch->in_callback) {
    php_warning ("Can't call curl_setopt_array from a callback");
    return false;
  }

  for (array <var>::const_iterator p = options.begin(); p != options.end(); ++p) {
    int option = p.get_key().to_int();

    if (!curl_setopt (ch, option, p.get_value())) {
      php_warning ("Can't set curl option \"%s\"", p.get_key().to_string().c_str());
      return false;
    }
  }
  
  return true;
}

var f$curl_exec (curl id) {
  curl_handler *ch = get_curl_handler (id);
  if (ch == NULL) {
    php_warning ("Wrong curl handler specified");
    return false;
  }

  if (ch->in_callback) {
    php_warning ("Can't call curl_exec from a callback");
    return false;
  }

  ch->result = string();
  ch->header = string();
  dl::enter_critical_section();//OK
  ch->error_no = curl_easy_perform (ch->cp);
  dl::leave_critical_section();

  if (ch->error_no != CURLE_OK && ch->error_no != CURLE_PARTIAL_FILE) {
    return false;
  }

  if (ch->return_transfer) {
    ch->result.finish_append();
    return ch->result;
  }

  return true;
}

var f$curl_getinfo (curl id, int option) {
  curl_handler *ch = get_curl_handler (id);
  if (ch == NULL) {
    php_warning ("Wrong curl handler specified");
    return false;
  }

  if (ch->in_callback) {
    php_warning ("Can't call curl_getinfo from a callback");
    return false;
  }

  CURL *cp = ch->cp;
  if (option == 0) {
    array <var> result (array_size (0, 26, false));

    long l_code;
    char *s_code;
    double d_code;

    //TODO: Need to enter critical section for safety
    if (curl_easy_getinfo (cp, CURLINFO_EFFECTIVE_URL, &s_code) == CURLE_OK && s_code != NULL) {
      result.set_value (string ("url", 3), string (s_code, strlen (s_code)));
    }
    if (curl_easy_getinfo (cp, CURLINFO_CONTENT_TYPE, &s_code) == CURLE_OK && s_code != NULL) {
      result.set_value (string ("content_type", 12), string (s_code, strlen (s_code)));
    }
    if (curl_easy_getinfo (cp, CURLINFO_RESPONSE_CODE, &l_code) == CURLE_OK) {
      result.set_value (string ("http_code", 9), (int)l_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_HEADER_SIZE, &l_code) == CURLE_OK) {
      result.set_value (string ("header_size", 11), (int)l_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_REQUEST_SIZE, &l_code) == CURLE_OK) {
      result.set_value (string ("request_size", 12), (int)l_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_FILETIME, &l_code) == CURLE_OK) {
      result.set_value (string ("filetime", 8), (int)l_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_SSL_VERIFYRESULT, &l_code) == CURLE_OK) {
      result.set_value (string ("ssl_verify_result", 17), (int)l_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_REDIRECT_COUNT, &l_code) == CURLE_OK) {
      result.set_value (string ("redirect_count", 14), (int)l_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_TOTAL_TIME, &d_code) == CURLE_OK) {
      result.set_value (string ("total_time", 10), d_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_NAMELOOKUP_TIME, &d_code) == CURLE_OK) {
      result.set_value (string ("namelookup_time", 15), d_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_CONNECT_TIME, &d_code) == CURLE_OK) {
      result.set_value (string ("connect_time", 12), d_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_PRETRANSFER_TIME, &d_code) == CURLE_OK) {
      result.set_value (string ("pretransfer_time", 16), d_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_SIZE_UPLOAD, &d_code) == CURLE_OK) {
      result.set_value (string ("size_upload", 11), d_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_SIZE_DOWNLOAD, &d_code) == CURLE_OK) {
      result.set_value (string ("size_download", 13), d_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_SPEED_DOWNLOAD, &d_code) == CURLE_OK) {
      result.set_value (string ("speed_download", 14), d_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_SPEED_UPLOAD, &d_code) == CURLE_OK) {
      result.set_value (string ("speed_upload", 12), d_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &d_code) == CURLE_OK) {
      result.set_value (string ("download_content_length", 23), d_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_CONTENT_LENGTH_UPLOAD, &d_code) == CURLE_OK) {
      result.set_value (string ("upload_content_length", 21), d_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_STARTTRANSFER_TIME, &d_code) == CURLE_OK) {
      result.set_value (string ("starttransfer_time", 18), d_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_REDIRECT_TIME, &d_code) == CURLE_OK) {
      result.set_value (string ("redirect_time", 13), d_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_REDIRECT_URL, &s_code) == CURLE_OK && s_code != NULL) {
      result.set_value (string ("redirect_url", 12), string (s_code, strlen (s_code)));
    }
    if (curl_easy_getinfo (cp, CURLINFO_PRIMARY_IP, &s_code) == CURLE_OK && s_code != NULL) {
      result.set_value (string ("primary_ip", 10), string (s_code, strlen (s_code)));
    }
    if (curl_easy_getinfo (cp, CURLINFO_PRIMARY_PORT, &l_code) == CURLE_OK) {
      result.set_value (string ("primary_port", 12), (int)l_code);
    }
    if (curl_easy_getinfo (cp, CURLINFO_LOCAL_IP, &s_code) == CURLE_OK && s_code != NULL) {
      result.set_value (string ("local_ip", 8), string (s_code, strlen (s_code)));
    }
    if (curl_easy_getinfo (cp, CURLINFO_LOCAL_PORT, &l_code) == CURLE_OK) {
      result.set_value (string ("local_port", 10), (int)l_code);
    }
    result.set_value (string ("request_header", 14), ch->header);

    return result;
  } else if (option == CURLOPT_INFO_HEADER_OUT) {
    return ch->header;
  } else {
    const int CURLINFO_FIRST_OPTION_NUMBER = 100000;
    const static CURLINFO curlinfo_options[] = {
        CURLINFO_EFFECTIVE_URL,
        CURLINFO_HTTP_CODE,
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
        CURLINFO_CONTENT_TYPE};
    const int CURLINFO_OPTIONS_COUNT = sizeof (curlinfo_options) / sizeof (curlinfo_options[0]);

    option -= CURLINFO_FIRST_OPTION_NUMBER;
    if ((unsigned int)option > (unsigned int)CURLINFO_OPTIONS_COUNT) {
      php_warning ("Wrong parameter option %d in function curl_getinfo", option + CURLINFO_FIRST_OPTION_NUMBER);
      return false;
    }
    CURLINFO curl_option = curlinfo_options[option];

    int type = CURLINFO_TYPEMASK & curl_option;
    switch (type) {
      case CURLINFO_STRING: {
        char *s_code = NULL;

        dl::enter_critical_section();//OK
        if (curl_easy_getinfo (cp, curl_option, &s_code) == CURLE_OK && s_code != NULL) {
          dl::leave_critical_section();
          return string (s_code, strlen (s_code));
        }
        dl::leave_critical_section();

        return false;
      }
      case CURLINFO_LONG: {
        long code = 0;

        dl::enter_critical_section();//OK
        if (curl_easy_getinfo (cp, curl_option, &code) == CURLE_OK) {
          dl::leave_critical_section();
          if ((long)INT_MIN <= code && code <= (long)INT_MAX) {
            return (int)code;
          }

          return f$strval (Long ((long long)code));
        }
        dl::leave_critical_section();

        return false;
      }
      case CURLINFO_DOUBLE: {
        double code = 0.0;

        dl::enter_critical_section();//OK
        if (curl_easy_getinfo (cp, curl_option, &code) == CURLE_OK) {
          dl::leave_critical_section();
          return code;
        }
        dl::leave_critical_section();

        return false;
      }
      default:
        php_assert (0);
        exit (1);
    }
  }

  php_assert (0);
  exit (1);
}

string f$curl_error (curl id) {
  curl_handler *ch = get_curl_handler (id);

  if (ch == NULL) {
    php_warning ("Wrong curl handler specified");
    return string();
  }

  if (ch->in_callback) {
    php_warning ("Can't call curl_error from a callback");
    return string();
  }

  if (ch->error_no == CURLE_OK) {
    return string();
  }

  return string (ch->error, strlen (ch->error));
}

int f$curl_errno (curl id) {
  curl_handler *ch = get_curl_handler (id);

  if (ch == NULL) {
    php_warning ("Wrong curl handler specified");
    return 0;
  }

  if (ch->in_callback) {
    php_warning ("Can't call curl_errno from a callback");
    return 0;
  }

  return ch->error_no;
}

static void curl_close (curl_handler *ch) {
  dl::enter_critical_section();//OK
  curl_easy_cleanup (ch->cp);
  dl::leave_critical_section();

  const curl_handler *const_ch = ch;
  for (array <curl_slist *>::const_iterator p = const_ch->slists_to_free.begin(); p != const_ch->slists_to_free.end(); ++p) {
    curl_slist *v = p.get_value();
    php_assert (v != NULL);
    dl::enter_critical_section();//OK
    curl_slist_free_all (v);
    dl::leave_critical_section();
  }

  for (array <curl_httppost *>::const_iterator p = const_ch->httpposts_to_free.begin(); p != const_ch->httpposts_to_free.end(); ++p) {
    curl_httppost *v = p.get_value();
    php_assert (v != NULL);
    dl::enter_critical_section();//OK
    curl_formfree (v);
    dl::leave_critical_section();
  }

  ch->~curl_handler();
  dl::deallocate (static_cast <void *> (ch), sizeof (curl_handler));
}

void f$curl_close (curl id) {
  curl_handler *ch = get_curl_handler (id);

  if (ch == NULL) {
    php_warning ("Wrong curl handler specified");
    return;
  }

  if (ch->in_callback) {
    php_warning ("Can't close cURL handle from a callback");
    return;
  }

  curl_handlers->set_value (id - 1, NULL);

  curl_close (ch);
}



static void *malloc_replace (size_t x) {
  size_t real_allocate = x + sizeof (size_t);
  php_assert (real_allocate >= sizeof (size_t));
  void *p = dl::allocate (real_allocate);
  if (p == NULL) {
    return NULL;//We believe that curl can handle this
  }
  *static_cast <size_t *> (p) = real_allocate;
  return static_cast <void *> (static_cast <char *> (p) + sizeof (size_t));
}

static void free_replace (void *p) {
  if (p == NULL) {
    return;
  }

  p = static_cast <void *>(static_cast <char *> (p) - sizeof (size_t));
  php_assert (dl::memory_begin <= (size_t)p && (size_t)p < dl::memory_end);
  dl::deallocate (p, *static_cast <size_t *> (p));
}

static void *realloc_replace (void *p, size_t x) {
  if (p == NULL) {
    return malloc_replace (x);
  }

  if (x == 0) {
    free_replace (p);
    return NULL;
  }

  void *real_p = static_cast <void *> (static_cast <char *> (p) - sizeof (size_t));
  php_assert (dl::memory_begin <= (size_t)real_p && (size_t)real_p < dl::memory_end);
  size_t old_size = *static_cast <size_t *> (real_p);

  void *new_p = malloc_replace (x);
  if (new_p == NULL) {
    return NULL;
  }
  memcpy (new_p, p, min (x, old_size));

  dl::deallocate (real_p, old_size);

  return new_p;
}

static char *strdup_replace (const char *str) {
  int len = strlen (str) + 1;
  char *res = static_cast <char *> (static_cast <void *> (malloc_replace (len)));
  if (res == NULL) {
    return NULL;
  }
  memcpy (res, str, len);
  return res;
}

static void *calloc_replace (size_t nmemb, size_t size) {
  void *res = malloc_replace (nmemb * size);
  if (res == NULL) {
    return NULL;
  }
  return memset (res, 0, nmemb * size);
}

void curl_init_static (void) {
  if (dl::query_num != curl_last_query_num) {
    new (curl_handlers_storage) array <curl_handler *>();

    php_assert (0 == CURLE_OK);

    dl::enter_critical_section();//OK
    CURLcode result = curl_global_init_mem (
        CURL_GLOBAL_ALL,
        malloc_replace,
        free_replace,
        realloc_replace,
        strdup_replace,
        calloc_replace);
    dl::leave_critical_section();

    if (result != CURLE_OK) {
      php_critical_error ("can't initialize curl");
    }

    curl_last_query_num = dl::query_num;
  }
}

void curl_free_static (void) {
  dl::enter_critical_section();//OK
  if (dl::query_num == curl_last_query_num) {
    const array <curl_handler *> *const_curl_handlers = curl_handlers;
    for (array <curl_handler *>::const_iterator p = const_curl_handlers->begin(); p != const_curl_handlers->end(); ++p) {
      if (p.get_value() != NULL) {
        curl_close (p.get_value());
      }
    }

    curl_global_cleanup();

    curl_last_query_num--;
  }
  dl::leave_critical_section();
}
