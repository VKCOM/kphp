#include "runtime/openssl.h"

#include <cerrno>
#include <memory>
#include <netdb.h>
#include <openssl/asn1.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <poll.h>
#include <sys/time.h>
#include <unistd.h>

#include "common/crc32.h"
#include "common/resolver.h"
#include "common/smart_ptrs/unique_ptr_with_delete_function.h"
#include "common/wrappers/openssl.h"

#include "runtime/datetime.h"
#include "runtime/files.h"
#include "runtime/net_events.h"
#include "runtime/streams.h"
#include "runtime/string_functions.h"
#include "runtime/url.h"

array<string> f$hash_algos(void) {
  return array<string>(
    string("sha1", 4),
    string("sha256", 6),
    string("md5", 3));
}

string f$hash(const string &algo, const string &s, bool raw_output) {
  if (!strcmp(algo.c_str(), "sha256")) {
    string res;
    if (raw_output) {
      res.assign((dl::size_type)32, false);
    } else {
      res.assign((dl::size_type)64, false);
    }

    SHA256(reinterpret_cast <const unsigned char *> (s.c_str()), (unsigned long)s.size(), reinterpret_cast <unsigned char *> (res.buffer()));

    if (!raw_output) {
      for (int i = 31; i >= 0; i--) {
        res[2 * i + 1] = lhex_digits[res[i] & 15];
        res[2 * i] = lhex_digits[(res[i] >> 4) & 15];
      }
    }
    return res;
  } else if (!strcmp(algo.c_str(), "md5")) {
    return f$md5(s, raw_output);
  } else if (!strcmp(algo.c_str(), "sha1")) {
    return f$sha1(s, raw_output);
  }

  php_critical_error ("algo %s not supported in function hash", algo.c_str());
}

string f$hash_hmac(const string &algo, const string &data, const string &key, bool raw_output) {
  const EVP_MD *evp_md = NULL;
  int hash_len = 0;
  if (!strcmp(algo.c_str(), "sha1")) {
    evp_md = EVP_sha1();
    hash_len = 20;
  } else if (!strcmp(algo.c_str(), "sha256")) {
    evp_md = EVP_sha256();
    hash_len = 32;
  }

  if (evp_md != NULL) {
    string res;
    if (raw_output) {
      res.assign((dl::size_type)hash_len, false);
    } else {
      res.assign((dl::size_type)hash_len * 2, false);
    }

    unsigned int md_len;
    HMAC(evp_md, static_cast <const void *> (key.c_str()), (int)key.size(),
         reinterpret_cast <const unsigned char *> (data.c_str()), (int)data.size(),
         reinterpret_cast <unsigned char *> (res.buffer()), &md_len);
    php_assert (md_len == (unsigned int)hash_len);

    if (!raw_output) {
      for (int i = hash_len - 1; i >= 0; i--) {
        res[2 * i + 1] = lhex_digits[res[i] & 15];
        res[2 * i] = lhex_digits[(res[i] >> 4) & 15];
      }
    }
    return res;
  }

  php_critical_error ("unsupported algo = \"%s\" in function hash_hmac", algo.c_str());
  return string();
}

string f$sha1(const string &s, bool raw_output) {
  string res;
  if (raw_output) {
    res.assign((dl::size_type)20, false);
  } else {
    res.assign((dl::size_type)40, false);
  }

  SHA1(reinterpret_cast <const unsigned char *> (s.c_str()), (unsigned long)s.size(), reinterpret_cast <unsigned char *> (res.buffer()));

  if (!raw_output) {
    for (int i = 19; i >= 0; i--) {
      res[2 * i + 1] = lhex_digits[res[i] & 15];
      res[2 * i] = lhex_digits[(res[i] >> 4) & 15];
    }
  }
  return res;
}


string f$md5(const string &s, bool raw_output) {
  string res;
  if (raw_output) {
    res.assign((dl::size_type)16, false);
  } else {
    res.assign((dl::size_type)32, false);
  }

  MD5(reinterpret_cast <const unsigned char *> (s.c_str()), (unsigned long)s.size(), reinterpret_cast <unsigned char *> (res.buffer()));

  if (!raw_output) {
    for (int i = 15; i >= 0; i--) {
      res[2 * i + 1] = lhex_digits[res[i] & 15];
      res[2 * i] = lhex_digits[(res[i] >> 4) & 15];
    }
  }
  return res;
}

OrFalse<string> f$md5_file(const string &file_name, bool raw_output) {
  dl::enter_critical_section();//OK
  struct stat stat_buf;
  int read_fd = open(file_name.c_str(), O_RDONLY);
  if (read_fd < 0) {
    dl::leave_critical_section();
    return false;
  }
  if (fstat(read_fd, &stat_buf) < 0) {
    close(read_fd);
    dl::leave_critical_section();
    return false;
  }

  if (!S_ISREG (stat_buf.st_mode)) {
    close(read_fd);
    dl::leave_critical_section();
    php_warning("Regular file expected in function md5_file, \"%s\" is given", file_name.c_str());
    return false;
  }

  MD5_CTX c;
  php_assert (MD5_Init(&c) == 1);

  size_t size = stat_buf.st_size;
  while (size > 0) {
    size_t len = min(size, (size_t)PHP_BUF_LEN);
    if (read_safe(read_fd, php_buf, len) < (ssize_t)len) {
      break;
    }
    php_assert (MD5_Update(&c, static_cast <const void *> (php_buf), (unsigned long)len) == 1);
    size -= len;
  }
  close(read_fd);
  php_assert (MD5_Final(reinterpret_cast <unsigned char *> (php_buf), &c) == 1);
  dl::leave_critical_section();

  if (size > 0) {
    php_warning("Error while reading file \"%s\"", file_name.c_str());
    return false;
  }

  if (!raw_output) {
    string res(32, false);
    for (int i = 15; i >= 0; i--) {
      res[2 * i + 1] = lhex_digits[php_buf[i] & 15];
      res[2 * i] = lhex_digits[(php_buf[i] >> 4) & 15];
    }
    return res;
  } else {
    return string(php_buf, 16);
  }
}

int f$crc32(const string &s) {
  return compute_crc32(static_cast <const void *> (s.c_str()), s.size());
}

int f$crc32_file(const string &file_name) {
  dl::enter_critical_section();//OK
  struct stat stat_buf;
  int read_fd = open(file_name.c_str(), O_RDONLY);
  if (read_fd < 0) {
    dl::leave_critical_section();
    return -1;
  }
  if (fstat(read_fd, &stat_buf) < 0) {
    close(read_fd);
    dl::leave_critical_section();
    return -1;
  }

  if (!S_ISREG (stat_buf.st_mode)) {
    close(read_fd);
    dl::leave_critical_section();
    php_warning("Regular file expected in function crc32_file, \"%s\" is given", file_name.c_str());
    return -1;
  }

  int res = -1;
  size_t size = stat_buf.st_size;
  while (size > 0) {
    size_t len = min(size, (size_t)PHP_BUF_LEN);
    if (read_safe(read_fd, php_buf, len) < (ssize_t)len) {
      break;
    }
    res = crc32_partial(php_buf, (int)len, res);
    size -= len;
  }
  close(read_fd);
  dl::leave_critical_section();

  if (size > 0) {
    return -1;
  }

  return res ^ -1;
}


static char openssl_pkey_storage[sizeof(array<EVP_PKEY *>)];
static array<EVP_PKEY *> *openssl_pkey = (array<EVP_PKEY *> *)openssl_pkey_storage;
static long long openssl_pkey_last_query_num = -1;

static EVP_PKEY *openssl_get_evp(const string &key, const string &passphrase, bool is_public, bool *from_cache) {
  int num = 0;
  if (openssl_pkey_last_query_num == dl::query_num && key[0] == ':' && key[1] == ':' && sscanf(key.c_str() + 2, "%d", &num) == 1 && (unsigned int)num < (unsigned int)openssl_pkey
    ->count()) {
    *from_cache = true;
    return openssl_pkey->get_value(num);
  }

  dl::enter_critical_section();//OK
  BIO *in = BIO_new_mem_buf(static_cast <void *> (const_cast <char *> (key.c_str())), key.size());
  if (in == NULL) {
    dl::leave_critical_section();
    return NULL;
  }
  EVP_PKEY *evp_pkey;
  X509 *cert = (X509 *)PEM_ASN1_read_bio((d2i_of_void *)d2i_X509, PEM_STRING_X509, in, NULL, NULL, NULL);
  BIO_free(in);
  if (cert == NULL) {
    in = BIO_new_mem_buf(static_cast <void *> (const_cast <char *> (key.c_str())), key.size());
    if (in == NULL) {
      dl::leave_critical_section();
      return NULL;
    }
    evp_pkey = is_public ? PEM_read_bio_PUBKEY(in, NULL, NULL, NULL) : PEM_read_bio_PrivateKey(in, NULL, NULL, static_cast <void *> (const_cast <char *> (passphrase
      .c_str())));
    BIO_free(in);
  } else {
    evp_pkey = X509_get_pubkey(cert);
    X509_free(cert);
  }
/*
  ERR_load_crypto_strings();
  if (evp_pkey == NULL) {
    unsigned long val;
    while ((val = ERR_get_error())) {
      fprintf (stderr, "%s\n", ERR_error_string (val, NULL));
    }
  }
*/
  dl::leave_critical_section();

  *from_cache = false;
  return evp_pkey;
}

bool f$openssl_public_encrypt(const string &data, string &result, const string &key) {
  dl::enter_critical_section();//OK
  bool from_cache;
  EVP_PKEY *pkey = openssl_get_evp(key, string(), true, &from_cache);
  if (pkey == NULL) {
    dl::leave_critical_section();

    php_warning("Parameter key is not a valid public key");
    result = string();
    return false;
  }
  if (EVP_PKEY_id(pkey) != EVP_PKEY_RSA && EVP_PKEY_id(pkey) != EVP_PKEY_RSA2) {
    if (!from_cache) {
      EVP_PKEY_free(pkey);
    }
    dl::leave_critical_section();

    php_warning("Key type is neither RSA nor RSA2");
    result = string();
    return false;
  }

  int key_size = EVP_PKEY_size(pkey);
  php_assert (PHP_BUF_LEN >= key_size);

  if (RSA_public_encrypt((int)data.size(), reinterpret_cast <const unsigned char *> (data.c_str()),
                         reinterpret_cast <unsigned char *> (php_buf), EVP_PKEY_get0_RSA(pkey), RSA_PKCS1_PADDING) != key_size) {
    if (!from_cache) {
      EVP_PKEY_free(pkey);
    }
    dl::leave_critical_section();

    php_warning("RSA public encrypt failed");
    result = string();
    return false;
  }

  if (!from_cache) {
    EVP_PKEY_free(pkey);
  }
  dl::leave_critical_section();

  result = string(php_buf, key_size);
  return true;
}

bool f$openssl_public_encrypt(const string &data, var &result, const string &key) {
  string result_string;
  if (f$openssl_public_encrypt(data, result_string, key)) {
    result = result_string;
    return true;
  }
  result = var();
  return false;
}

bool f$openssl_private_decrypt(const string &data, string &result, const string &key) {
  dl::enter_critical_section();//OK
  bool from_cache;
  EVP_PKEY *pkey = openssl_get_evp(key, string(), false, &from_cache);
  if (pkey == NULL) {
    dl::leave_critical_section();
    php_warning("Parameter key is not a valid private key");
    return false;
  }
  if (EVP_PKEY_id(pkey) != EVP_PKEY_RSA && EVP_PKEY_id(pkey) != EVP_PKEY_RSA2) {
    if (!from_cache) {
      EVP_PKEY_free(pkey);
    }
    dl::leave_critical_section();
    php_warning("Key type is not an RSA nor RSA2");
    return false;
  }

  int key_size = EVP_PKEY_size(pkey);
  php_assert (PHP_BUF_LEN >= key_size);

  int len = RSA_private_decrypt((int)data.size(), reinterpret_cast <const unsigned char *> (data.c_str()),
                                reinterpret_cast <unsigned char *> (php_buf), EVP_PKEY_get0_RSA(pkey), RSA_PKCS1_PADDING);
  if (!from_cache) {
    EVP_PKEY_free(pkey);
  }
  dl::leave_critical_section();
  if (len == -1) {
    //php_warning ("RSA private decrypt failed"); 
    result = string();
    return false;
  }

  result.assign(php_buf, len);
  return true;
}

bool f$openssl_private_decrypt(const string &data, var &result, const string &key) {
  string result_string;
  if (f$openssl_private_decrypt(data, result_string, key)) {
    result = result_string;
    return true;
  }
  result = var();
  return false;
}

OrFalse<string> f$openssl_pkey_get_private(const string &key, const string &passphrase) {
  dl::enter_critical_section();//NOT OK: openssl_pkey

  bool from_cache;
  EVP_PKEY *pkey = openssl_get_evp(key, passphrase, false, &from_cache);

  if (pkey == NULL) {
    dl::leave_critical_section();

    php_warning("Parameter key is not a valid key or passphrase is not a valid password");
    return false;
  }

  if (from_cache) {
    dl::leave_critical_section();
    return key;
  }

  if (dl::query_num != openssl_pkey_last_query_num) {
    new(openssl_pkey_storage) array<EVP_PKEY *>();
    openssl_pkey_last_query_num = dl::query_num;
  }

  string result(2, ':');
  result.append(openssl_pkey->count());
  openssl_pkey->push_back(pkey);
  dl::leave_critical_section();

  return result;
}

static const EVP_MD *openssl_algo_to_evp_md(openssl_algo algo) {
  switch (algo) {
    case OPENSSL_ALGO_SHA1:
      return EVP_sha1();

# ifndef OPENSSL_NO_MD5
    case OPENSSL_ALGO_MD5:
      return EVP_md5();
# endif

# ifndef OPENSSL_NO_MD4
    case OPENSSL_ALGO_MD4:
      return EVP_md4();
#endif

#ifndef OPENSSL_NO_MD2
      case OPENSSL_ALGO_MD2:  return EVP_md2();
#endif

    case OPENSSL_ALGO_SHA224:
      return EVP_sha224();
    case OPENSSL_ALGO_SHA256:
      return EVP_sha256();
    case OPENSSL_ALGO_SHA384:
      return EVP_sha384();
    case OPENSSL_ALGO_SHA512:
      return EVP_sha512();

#ifndef OPENSSL_NO_RMD160
    case OPENSSL_ALGO_RMD160:
      return EVP_ripemd160();
#endif

    default:
      return nullptr;
  }
}

int f$openssl_verify(const string &data, const string &signature, const string &pub_key_id, int algo) {
  int err = 0;
  EVP_PKEY *pkey;
  EVP_MD_CTX *md_ctx;
  dl::enter_critical_section();
  const EVP_MD *mdtype = openssl_algo_to_evp_md(static_cast<openssl_algo>(algo));

  if (signature.size() > UINT_MAX) {
    php_warning("signature is too long");
    dl::leave_critical_section();
    return 0;
  }

  bool from_cache;
  pkey = openssl_get_evp(pub_key_id, string("", 0), true, &from_cache);
  if (pkey == NULL) {
    php_warning("supplied key param cannot be converted into a public key");
    dl::leave_critical_section();
    return 0;
  }

  md_ctx = EVP_MD_CTX_create();
  if (md_ctx == NULL ||
      !EVP_VerifyInit (md_ctx, mdtype) ||
      !EVP_VerifyUpdate (md_ctx, data.c_str(), data.size()) ||
      (err = EVP_VerifyFinal(md_ctx, (unsigned char *)signature.c_str(), (unsigned int)signature.size(), pkey)) < 0) {
  }
  EVP_MD_CTX_destroy(md_ctx);
  EVP_PKEY_free(pkey);
  dl::leave_critical_section();
  return err;
}

OrFalse<string> f$openssl_random_pseudo_bytes(int length) {
  if (length <= 0) {
    return false;
  }
  string buffer(length, ' ');
  struct timeval tv;

  gettimeofday(&tv, NULL);
  RAND_add(&tv, sizeof(tv), 0.0);

  if (RAND_bytes((unsigned char *)buffer.buffer(), length) <= 0) {
    return false;
  }
  return buffer;
}


struct ssl_connection {
  int sock;
  bool is_blocking;

  SSL *ssl_handle;
  SSL_CTX *ssl_ctx;
};

static char ssl_connections_storage[sizeof(array<ssl_connection>)];
static array<ssl_connection> *ssl_connections = reinterpret_cast <array<ssl_connection> *> (ssl_connections_storage);
static long long ssl_connections_last_query_num = -1;


const int DEFAULT_SOCKET_TIMEOUT = 60;

static const char *ssl_get_error_string(void) {
  static_SB.clean();
  while (unsigned long error_code = ERR_get_error()) {
    static_SB << "Error " << (int)error_code << ": [" << ERR_error_string(error_code, NULL) << "]\n";
  }
  return static_SB.c_str();
}


static Stream ssl_stream_socket_client(const string &url, int &error_number, string &error_description, double timeout, int flags __attribute__((unused)), const var &options) {
#define RETURN(dump_error_stack)                        \
  if (dump_error_stack) {                               \
    php_warning ("%s: %s", error_description.c_str(),   \
                           ssl_get_error_string());     \
  } else {                                              \
    php_warning ("%s", error_description.c_str());      \
  }                                                     \
  if (ssl_handle != NULL) {                             \
    SSL_free (ssl_handle);                              \
  }                                                     \
  if (ssl_ctx != NULL) {                                \
    SSL_CTX_free (ssl_ctx);                             \
  }                                                     \
  if (sock != -1) {                                     \
    close (sock);                                       \
  }                                                     \
  dl::leave_critical_section();                         \
  return false

#define RETURN_ERROR(dump_error_stack, error_no, error) \
  error_number = error_no;                              \
  error_description = CONST_STRING(error);              \
  RETURN(dump_error_stack)

#define RETURN_ERROR_FORMAT(dump_error_stack, error_no, format, ...) \
  error_number = error_no;                                           \
  error_description = f$sprintf (                                    \
    array <var> (CONST_STRING(format), __VA_ARGS__));                \
  RETURN(dump_error_stack)

  if (timeout < 0) {
    timeout = DEFAULT_SOCKET_TIMEOUT;
  }
  double end_time = microtime_monotonic() + timeout;

  int sock = -1;

  SSL *ssl_handle = NULL;
  SSL_CTX *ssl_ctx = NULL;

  var parsed_url = f$parse_url(url);
  string host = f$strval(parsed_url.get_value(string("host", 4)));
  int port = f$intval(parsed_url.get_value(string("port", 4)));

  //getting connection options
#define GET_OPTION(option_name) options.get_value (string (option_name, sizeof (option_name) - 1))
  bool verify_peer = GET_OPTION("verify_peer").to_bool();
  var verify_depth_var = GET_OPTION("verify_depth");
  int verify_depth = verify_depth_var.to_int();
  if (verify_depth_var.is_null()) {
    verify_depth = -1;
  }
  string cafile = GET_OPTION("cafile").to_string();
  string capath = GET_OPTION("capath").to_string();
  string cipher_list = GET_OPTION("ciphers").to_string();
  string local_cert = GET_OPTION("local_cert").to_string();
  OrFalse<string> local_cert_file_path = local_cert.empty() ? false : f$realpath(local_cert);
#undef GET_OPTION

  if (host.empty()) {
    RETURN_ERROR_FORMAT(false, -1, "Wrong host specified in url \"%s\"", url);
  }

  if (port <= 0 || port >= 65536) {
    RETURN_ERROR_FORMAT(false, -2, "Wrong port specified in url \"%s\"", url);
  }

  struct hostent *h;
  if (!(h = kdb_gethostbyname(host.c_str())) || !h->h_addr_list || !h->h_addr_list[0]) {
    RETURN_ERROR_FORMAT(false, -3, "Can't resolve host \"%s\"", host);
  }

  struct sockaddr_storage addr;
  addr.ss_family = h->h_addrtype;
  int addrlen;
  switch (h->h_addrtype) {
    case AF_INET:
      php_assert (h->h_length == 4);
      (reinterpret_cast <struct sockaddr_in *> (&addr))->sin_port = htons(port);
      (reinterpret_cast <struct sockaddr_in *> (&addr))->sin_addr = *(struct in_addr *)h->h_addr;
      addrlen = sizeof(struct sockaddr_in);
      break;
    case AF_INET6:
      php_assert (h->h_length == 16);
      (reinterpret_cast <struct sockaddr_in6 *> (&addr))->sin6_port = htons(port);
      memcpy(&(reinterpret_cast <struct sockaddr_in6 *> (&addr))->sin6_addr, h->h_addr, h->h_length);
      addrlen = sizeof(struct sockaddr_in6);
      break;
    default:
      //there is no other known address types
      php_assert (0);
  }


  dl::enter_critical_section();//OK
  sock = socket(h->h_addrtype, SOCK_STREAM, 0);
  if (sock == -1) {
    RETURN_ERROR(false, -4, "Can't create tcp socket");
  }

  php_assert (fcntl(sock, F_SETFL, O_NONBLOCK) == 0);

  if (connect(sock, reinterpret_cast <const sockaddr *> (&addr), addrlen) == -1) {
    if (errno != EINPROGRESS) {
      RETURN_ERROR(false, -5, "Can't connect to tcp socket");
    }

    pollfd poll_fds;
    poll_fds.fd = sock;
    poll_fds.events = POLLIN | POLLERR | POLLHUP | POLLOUT | POLLPRI;

    double left_time = end_time - microtime_monotonic();
    if (left_time <= 0 || poll(&poll_fds, 1, timeout_convert_to_ms(left_time)) <= 0) {
      RETURN_ERROR(false, -6, "Can't connect to tcp socket");
    }
  }

  ERR_clear_error();

  ssl_ctx = SSL_CTX_new(TLSv1_client_method());
  if (ssl_ctx == NULL) {
    RETURN_ERROR(true, -7, "Failed to create an SSL context");
  }
//  SSL_CTX_set_options (ssl_ctx, SSL_OP_ALL); don't want others bugs workarounds
  SSL_CTX_set_mode (ssl_ctx, SSL_MODE_AUTO_RETRY | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER | SSL_MODE_ENABLE_PARTIAL_WRITE);

  if (verify_peer) {
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, NULL);

    if (verify_depth != -1) {
      SSL_CTX_set_verify_depth(ssl_ctx, verify_depth);
    }

    if (cafile.empty() && capath.empty()) {
      SSL_CTX_set_default_verify_paths(ssl_ctx);
    } else {
      if (SSL_CTX_load_verify_locations(ssl_ctx, cafile.empty() ? NULL : cafile.c_str(), capath.empty() ? NULL : capath.c_str()) == 0) {
        RETURN_ERROR_FORMAT(true, -8, "Failed to load verify locations \"%s\" and \"%s\"", cafile, capath);
      }
    }
  } else {
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, NULL);
  }

  if (SSL_CTX_set_cipher_list(ssl_ctx, cipher_list.empty() ? "DEFAULT" : cipher_list.c_str()) == 0) {
    RETURN_ERROR_FORMAT(true, -9, "Failed to set cipher list \"%s\"", cipher_list);
  }

  if (f$boolval(local_cert_file_path)) {
    if (SSL_CTX_use_certificate_chain_file(ssl_ctx, local_cert_file_path.val().c_str()) != 1) {
      RETURN_ERROR_FORMAT(true, -10, "Failed to use certificate from local_cert \"%s\"", local_cert);
    }

    if (SSL_CTX_use_PrivateKey_file(ssl_ctx, local_cert_file_path.val().c_str(), SSL_FILETYPE_PEM) != 1) {
      RETURN_ERROR_FORMAT(true, -11, "Failed to use private key from local_cert \"%s\"", local_cert);
    }

    if (SSL_CTX_check_private_key(ssl_ctx) != 1) {
      RETURN_ERROR(true, -12, "Private key doesn't match certificate");
    }
  }

  ssl_handle = SSL_new(ssl_ctx);
  if (ssl_handle == NULL) {
    RETURN_ERROR(true, -13, "Failed to create an SSL handle");
  }

  if (!SSL_set_fd(ssl_handle, sock)) {
    RETURN_ERROR(true, -14, "Failed to set fd");
  }

#if OPENSSL_VERSION_NUMBER >= 0x0090806fL && !defined(OPENSSL_NO_TLSEXT)
  SSL_set_tlsext_host_name (ssl_handle, host.c_str());
#endif

  SSL_set_connect_state(ssl_handle); //TODO remove

  while (true) {
    int connect_result = SSL_connect(ssl_handle);
    if (connect_result == 1) {
      if (verify_peer) {
        X509 *peer_cert = SSL_get_peer_certificate(ssl_handle);
        if (peer_cert == NULL) {
          SSL_shutdown(ssl_handle);
          RETURN_ERROR(false, -15, "Failed to get peer certificate");
        }

        X509_free(peer_cert);
        php_assert (SSL_get_verify_result(ssl_handle) == X509_V_OK);
      }

      break;
    }

    int error = SSL_get_error(ssl_handle, connect_result);
    if (error != SSL_ERROR_WANT_READ && error != SSL_ERROR_WANT_WRITE) {
      RETURN_ERROR(true, -16, "Failed to do SSL_connect");
    }

    pollfd poll_fds;
    poll_fds.fd = sock;
    poll_fds.events = POLLIN | POLLERR | POLLHUP | POLLOUT | POLLPRI;

    double left_time = end_time - microtime_monotonic();
    if (left_time <= 0 || poll(&poll_fds, 1, timeout_convert_to_ms(left_time)) <= 0) {
      RETURN_ERROR(false, -17, "Can't establish SSL connect due to timeout");
    }
  }

  php_assert (fcntl(sock, F_SETFL, 0) == 0);
  dl::leave_critical_section();

  if (dl::query_num != ssl_connections_last_query_num) {
    new(ssl_connections_storage) array<ssl_connection>();

    ssl_connections_last_query_num = dl::query_num;
  }

  ssl_connection result;
  result.sock = sock;
  result.is_blocking = true;

  result.ssl_handle = ssl_handle;
  result.ssl_ctx = ssl_ctx;

  string stream_key = url;
  if (ssl_connections->has_key(stream_key)) {
    int try_count;
    for (try_count = 0; try_count < 3; try_count++) {
      stream_key = url;
      stream_key.append("#", 1);
      stream_key.append(string(rand()));

      if (!ssl_connections->has_key(stream_key)) {
        break;
      }
    }

    if (try_count == 3) {
      SSL_shutdown(ssl_handle);
      RETURN_ERROR(false, -18, "Can't generate stream_name in 3 tries. Something is definitely wrong.");
    }
  }

  dl::enter_critical_section();//NOT OK: ssl_connections
  ssl_connections->set_value(stream_key, result);
  dl::leave_critical_section();

  return stream_key;
#undef RETURN
#undef RETURN_ERROR
#undef RETURN_ERROR_FORMAT
}

static bool ssl_context_set_option(var &context_ssl, const string &option, const var &value) {
  if (STRING_EQUALS(option, "verify_peer") ||
      STRING_EQUALS(option, "verify_depth") ||
      STRING_EQUALS(option, "cafile") ||
      STRING_EQUALS(option, "capath") ||
      STRING_EQUALS(option, "local_cert") ||
      STRING_EQUALS(option, "ciphers")) {
    context_ssl[option] = value;
    return true;
  }

  php_warning("Option \"%s\" for wrapper ssl is not supported", option.c_str());
  return false;
}

ssl_connection *get_connection(const Stream &stream) {
  if (dl::query_num != ssl_connections_last_query_num || !ssl_connections->has_key(stream.to_string())) {
    php_warning("Connection to \"%s\" not found", stream.to_string().c_str());
    return NULL;
  }

  return &(*ssl_connections)[stream.to_string()];
}

static void ssl_do_shutdown(ssl_connection *c) {
  php_assert (dl::in_critical_section > 0);

  if (c->sock == -1) {
    return;
  }

  if (!c->is_blocking) {
    php_assert (fcntl(c->sock, F_SETFL, 0) == 0);
  }

  php_assert (c->ssl_ctx != NULL);
  php_assert (c->ssl_handle != NULL);

  SSL_shutdown(c->ssl_handle);
  SSL_free(c->ssl_handle);
  SSL_CTX_free(c->ssl_ctx);
  close(c->sock);

  c->sock = -1;
  c->ssl_ctx = NULL;
  c->ssl_handle = NULL;
}

static bool process_ssl_error(ssl_connection *c, int result) {
  php_assert (dl::in_critical_section > 0);

  int error = SSL_get_error(c->ssl_handle, result);
  switch (error) {
    case SSL_ERROR_NONE:
      php_assert (0);
      return false;
    case SSL_ERROR_ZERO_RETURN:
      if (c->sock != -1) {
        ssl_do_shutdown(c);
      } else {
        php_warning("SSL connection is already closed");
      }
      return false;
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      return true;
    case SSL_ERROR_WANT_CONNECT:
    case SSL_ERROR_WANT_ACCEPT:
    case SSL_ERROR_WANT_X509_LOOKUP:
      php_assert (0);
      return false;

    case SSL_ERROR_SYSCALL:
      if (ERR_peek_error() == 0) {
        if (result != 0) {
          if (errno == EAGAIN) {
            return true;
          }

          php_warning("SSL gets error %d: %s", errno, strerror(errno));
        } else {
          //socket was closed from other side, not an error
          ssl_do_shutdown(c);
        }
        return false;
      }
      /* fall through */
    default:
      php_warning("SSL gets error %d: %s", error, ssl_get_error_string());
      ssl_do_shutdown(c);
      return false;
  }
}

static OrFalse<int> ssl_fwrite(const Stream &stream, const string &data) {
  ssl_connection *c = get_connection(stream);
  if (c == NULL || c->sock == -1) {
    return false;
  }

  const void *data_ptr = static_cast <const void *> (data.c_str());
  int data_len = static_cast <int> (data.size());
  if (data_len == 0) {
    return 0;
  }

  dl::enter_critical_section();//OK
  ERR_clear_error();
  int written = SSL_write(c->ssl_handle, data_ptr, data_len);

  if (written <= 0) {
    bool ok = process_ssl_error(c, written);
    if (ok) {
      dl::leave_critical_section();
      php_assert (!c->is_blocking);//because of SSL_MODE_AUTO_RETRY
      return 0;
    }
    dl::leave_critical_section();
    return false;
  } else {
    dl::leave_critical_section();
    if (c->is_blocking) {
      php_assert (written == data_len);
    } else {
      php_assert (written <= data_len);
    }
    return written;
  }
}

static OrFalse<string> ssl_fread(const Stream &stream, int length) {
  if (length <= 0) {
    php_warning("Parameter length in function fread must be positive");
    return false;
  }

  ssl_connection *c = get_connection(stream);
  if (c == NULL || c->sock == -1) {
    return false;
  }

  string res(length, false);
  dl::enter_critical_section();//OK
  ERR_clear_error();
  int res_size = SSL_read(c->ssl_handle, &res[0], length);
  if (res_size <= 0) {
    bool ok = process_ssl_error(c, res_size);
    if (ok) {
      dl::leave_critical_section();
      php_assert (!c->is_blocking);//because of SSL_MODE_AUTO_RETRY

      return string();
    }
    dl::leave_critical_section();
    return false;
  } else {
    dl::leave_critical_section();
    php_assert (res_size <= length);
    res.shrink((dl::size_type)res_size);
    return res;
  }
}

static bool ssl_feof(const Stream &stream) {
  ssl_connection *c = get_connection(stream);
  if (c == NULL || c->sock == -1) {
    return true;
  }

  pollfd poll_fds;
  poll_fds.fd = c->sock;
  poll_fds.events = POLLIN | POLLERR | POLLHUP | POLLPRI;

  if (poll(&poll_fds, 1, 0) <= 0) {
    return false;//nothing to read for now
  }

  dl::enter_critical_section();//OK
  char b;
  ERR_clear_error();
  int res_size = SSL_peek(c->ssl_handle, &b, 1);
  if (res_size <= 0) {
    bool ok = process_ssl_error(c, res_size);
    if (ok) {
      dl::leave_critical_section();
      return false;
    }
    dl::leave_critical_section();

    return true;
  } else {
    dl::leave_critical_section();

    return false;
  }
}

static bool ssl_fclose(const Stream &stream) {
  const string &stream_key = stream.to_string();
  if (dl::query_num != ssl_connections_last_query_num || !ssl_connections->has_key(stream_key)) {
    return false;
  }

  dl::enter_critical_section();//NOT OK: ssl_connections
  ssl_do_shutdown(&(*ssl_connections)[stream_key]);
  ssl_connections->unset(stream_key);
  dl::leave_critical_section();
  return true;
}

bool ssl_stream_set_option(const Stream &stream, int option, int value) {
  ssl_connection *c = get_connection(stream);
  if (c == NULL) {
    return false;
  }

  switch (option) {
    case STREAM_SET_BLOCKING_OPTION: {
      bool is_blocking = value;
      if (c->is_blocking != is_blocking) {
        dl::enter_critical_section();//OK
        if (is_blocking) {
          php_assert (fcntl(c->sock, F_SETFL, 0) == 0);
        } else {
          php_assert (fcntl(c->sock, F_SETFL, O_NONBLOCK) == 0);
        }
        dl::leave_critical_section();

        c->is_blocking = is_blocking;
      }
      return true;
    }
    case STREAM_SET_WRITE_BUFFER_OPTION:
    case STREAM_SET_READ_BUFFER_OPTION:
      if (value != 0) {
//TODO
//        BIO_set_read_buffer_size (SSL_get_rbio (c->ssl_handle), value);
//        BIO_set_write_buffer_size (SSL_get_wbio (c->ssl_handle), value);

        php_warning("SSL wrapper doesn't support buffered input/output");
        return false;
      }
      return true;
    default:
      php_assert (0);
  }
  return false;
}

static int ssl_get_fd(const Stream &stream) {
  ssl_connection *c = get_connection(stream);
  if (c == NULL) {
    return -1;
  }

  if (c->sock == -1) {
    php_warning("Connection to \"%s\" already closed", stream.to_string().c_str());
  }
  return c->sock;
}

void openssl_init_static_once(void) {
  static stream_functions ssl_stream_functions;

  ssl_stream_functions.name = string("ssl", 3);
  ssl_stream_functions.fopen = NULL;
  ssl_stream_functions.fwrite = ssl_fwrite;
  ssl_stream_functions.fseek = NULL;
  ssl_stream_functions.ftell = NULL;
  ssl_stream_functions.fread = ssl_fread;
  ssl_stream_functions.fgetc = NULL;
  ssl_stream_functions.fgets = NULL;
  ssl_stream_functions.fpassthru = NULL;
  ssl_stream_functions.fflush = NULL;
  ssl_stream_functions.feof = ssl_feof;
  ssl_stream_functions.fclose = ssl_fclose;

  ssl_stream_functions.file_get_contents = NULL;
  ssl_stream_functions.file_put_contents = NULL;

  ssl_stream_functions.stream_socket_client = ssl_stream_socket_client;
  ssl_stream_functions.context_set_option = ssl_context_set_option;
  ssl_stream_functions.stream_set_option = ssl_stream_set_option;
  ssl_stream_functions.get_fd = ssl_get_fd;

  register_stream_functions(&ssl_stream_functions, false);

  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();
  OpenSSL_add_ssl_algorithms();
}

void openssl_init_static(void) {
}

void openssl_free_static(void) {
  dl::enter_critical_section();//OK
  if (dl::query_num == openssl_pkey_last_query_num) {
    const array<EVP_PKEY *> *const_openssl_pkey = openssl_pkey;
    for (array<EVP_PKEY *>::const_iterator p = const_openssl_pkey->begin(); p != const_openssl_pkey->end(); ++p) {
      EVP_PKEY_free(p.get_value());
    }
    openssl_pkey_last_query_num--;
  }

  if (dl::query_num == ssl_connections_last_query_num) {
    const array<ssl_connection> *const_ssl_connections = ssl_connections;
    for (array<ssl_connection>::const_iterator p = const_ssl_connections->begin(); p != const_ssl_connections->end(); ++p) {
      ssl_connection c = p.get_value();
      ssl_do_shutdown(&c);
    }
    ssl_connections_last_query_num--;
  }
  dl::leave_critical_section();
}


class X509_parser {
private:
  using BIO_ptr = vk::unique_ptr_with_delete_function<BIO, BIO_vfree>;
  using X509_ptr = vk::unique_ptr_with_delete_function<X509, X509_free>;
  using ASN1_TIME_ptr = vk::unique_ptr_with_delete_function<ASN1_TIME, ASN1_TIME_free>;
  using X509_STORE_ptr = vk::unique_ptr_with_delete_function<X509_STORE, X509_STORE_free>;
  using X509_STORE_CTX_ptr = vk::unique_ptr_with_delete_function<X509_STORE_CTX, X509_STORE_CTX_free>;

public:
  explicit X509_parser(const string &data) :
    x509_(get_x509_from_data(data)) {}

  array<var> get_subject(bool shortnames = true) const {
    php_assert(x509_.get());

    X509_NAME *subj = X509_get_subject_name(x509_.get());

    return get_entries_of(subj, shortnames);
  }

  string get_subject_name() const {
    php_assert(x509_.get());

    auto issuer_name = X509_get_subject_name(x509_.get());
    auto issuer = X509_NAME_oneline(issuer_name, nullptr, 0);

    string res(issuer, strlen(issuer));
    OPENSSL_free(issuer);

    return res;
  }

  string get_hash() const {
    php_assert(x509_.get());

    static char buf[9];
    snprintf(buf, sizeof(buf), "%08lx", X509_subject_name_hash(x509_.get()));

    return string(buf, 8);
  }

  array<var> get_issuer(bool shortnames = true) const {
    php_assert(x509_.get());

    X509_NAME *issuer_name = X509_get_issuer_name(x509_.get());

    return get_entries_of(issuer_name, shortnames);
  }

  int get_version() const {
    php_assert(x509_.get());

    auto version = static_cast<int>(X509_get_version(x509_.get()));
    return version;
  }

  int get_time_not_before() const {
    php_assert(x509_.get());

    auto asn_time_not_before = X509_getm_notBefore(x509_.get());
    return (int)convert_asn1_time(asn_time_not_before);
  }

  int get_time_not_after() const {
    php_assert(x509_.get());

    auto asn_time_not_after = X509_getm_notAfter(x509_.get());
    return (int)convert_asn1_time(asn_time_not_after);
  }

  array<var> get_purposes(bool shortnames = true) const {
    php_assert(x509_.get());

    array<var> res;
    for (int i = 0; i < X509_PURPOSE_get_count(); i++) {
      array<var> purposes;

      X509_PURPOSE *purp = X509_PURPOSE_get0(i);
      int id = X509_PURPOSE_get_id(purp);

      auto purpset = static_cast<bool>(X509_check_purpose(x509_.get(), id, 0));
      purposes.push_back(purpset);

      purpset = static_cast<bool>(X509_check_purpose(x509_.get(), id, 1));
      purposes.push_back(purpset);

      char *pname = shortnames ? X509_PURPOSE_get0_sname(purp) : X509_PURPOSE_get0_name(purp);
      purposes.push_back(string(pname));

      res.set_value(id, purposes);
    }

    return res;
  }

  OrFalse<array<var>> parse(bool shortnames = true) const {
    if (!x509_) {
      return false;
    }

    return std::initializer_list<std::pair<string, var>>
      {{string("name"),             get_subject_name()},
       {string("subject"),          get_subject(shortnames)},
       {string("hash"),             get_hash()},
       {string("issuer"),           get_issuer(shortnames)},
       {string("version"),          get_version()},
       {string("validFrom_time_t"), get_time_not_before()},
       {string("validTo_time_t"),   get_time_not_after()},
       {string("purposes"),         get_purposes(shortnames)}};
  }

  var check_purpose(int purpose) const {
    if (!x509_) {
      return -1;
    }

    X509_STORE_CTX_ptr csc{X509_STORE_CTX_new()};

    if (csc == NULL) {
      return -1;
    }
    X509_STORE_ptr store{X509_STORE_new()};
    if (store == NULL) {
      return -1;
    }

    X509_STORE_set_default_paths(store.get());

    if (!X509_STORE_CTX_init(csc.get(), store.get(), x509_.get(), NULL)) {
      return -1;
    }
    // return value is not checked, as in php
    X509_STORE_CTX_set_purpose(csc.get(), purpose);
    int ret = X509_verify_cert(csc.get());
    if (ret != 0 && ret != 1) {
      return ret;
    }
    return static_cast<bool>(ret);
  }

private:
  X509_ptr get_x509_from_data(const string &data) {
    BIO_ptr certBio{BIO_new(BIO_s_mem())};

    if (!certBio) {
      return nullptr;
    }

    bool write_success = BIO_write(certBio.get(), data.c_str(), static_cast<int>(data.size())) != 0;

    if (!write_success) {
      return nullptr;
    }

    return X509_ptr(PEM_read_bio_X509(certBio.get(), nullptr, nullptr, nullptr));
  }

  array<var> get_entries_of(X509_NAME *name, bool shortnames = true) const {
    array<var> res;

    int count_of_entries = X509_NAME_entry_count(name);

    for (int i = 0; i < count_of_entries; i++) {
      X509_NAME_ENTRY *entry = X509_NAME_get_entry(name, i);

      ASN1_OBJECT *key = X509_NAME_ENTRY_get_object(entry);
      ASN1_STRING *value = X509_NAME_ENTRY_get_data(entry);

      int nid = OBJ_obj2nid(key);
      const char *key_str = shortnames ? OBJ_nid2sn(nid) : OBJ_nid2ln(nid);

      auto value_str = reinterpret_cast<const char *>(ASN1_STRING_get0_data(value));
      int value_len = ASN1_STRING_length(value);

      res.set_value(string(key_str, strlen(key_str)), string(value_str, value_len));
    }

    return res;
  }

  time_t convert_asn1_time(ASN1_TIME *expires) const {
    ASN1_TIME_ptr epoch{ASN1_TIME_new()};
    ASN1_TIME_set_string(epoch.get(), "700101000000");
    int days, seconds;
    ASN1_TIME_diff(&days, &seconds, epoch.get(), expires);

    return (days * 24 * 60 * 60) + seconds;
  }


private:
  X509_ptr x509_;
};

OrFalse<array<var>> f$openssl_x509_parse(const string &data, bool shortnames /* =true */) {
  return X509_parser(data).parse(shortnames);
}

var f$openssl_x509_checkpurpose(const string &data, int purpose) {
  return X509_parser(data).check_purpose(purpose);
}


