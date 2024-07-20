#include <chrono>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>

#include "runtime/sessions.h"
#include "runtime/interface.h"
#include "runtime/php-script-globals.h"
#include "runtime/files.h"
#include "runtime/serialize-functions.h"
#include "runtime/misc.h"
#include "common/wrappers/to_array.h"
#include "runtime/url.h"
#include "runtime/math_functions.h"

namespace sessions {

static mixed get_sparam(const char *key) noexcept;
static mixed get_sparam(const string &key) noexcept;
static void set_sparam(const char *key, const mixed &value) noexcept;
static void set_sparam(const string &key, const mixed &value) noexcept;
static void reset_sparams() noexcept;
static void initialize_sparams(const array<mixed> &options) noexcept;

static bool session_start();
static bool session_initialize();
static bool session_generate_id();
static bool session_valid_id(const string &id);
static bool session_abort();
static bool session_reset_id();
static bool session_send_cookie();
static int session_gc(const bool &immediate);
static bool session_flush();
static string session_encode();
static bool session_decode(const string &data);

static bool session_open();
static bool session_read();
static bool session_write();
static void session_close();

constexpr static auto S_READ_CLOSE = "read_and_close";
constexpr static auto S_ID = "session_id";
constexpr static auto S_FD = "handler";
constexpr static auto S_STATUS = "session_status";
constexpr static auto S_DIR = "save_path";
constexpr static auto S_PATH = "session_path";
constexpr static auto S_NAME = "name";
constexpr static auto S_CTIME = "session_ctime";
constexpr static auto S_LIFETIME = "gc_maxlifetime";
constexpr static auto S_PROBABILITY = "gc_probability";
constexpr static auto S_DIVISOR = "gc_divisor";
constexpr static auto S_SEND_COOKIE = "send_cookie";
constexpr static auto C_PATH = "cookie_path";
constexpr static auto C_LIFETIME = "cookie_lifetime";
constexpr static auto C_DOMAIN = "cookie_domain";
constexpr static auto C_SECURE = "cookie_secure";
constexpr static auto C_HTTPONLY = "cookie_httponly";

// TO-DO: reconsider it
const auto skeys = vk::to_array<std::pair<const char *, const mixed>>({
	{S_READ_CLOSE, false},
	{S_DIR, string((getenv("TMPDIR") != NULL) ? getenv("TMPDIR") : "/tmp/").append("sessions/")},
	{S_NAME, string("PHPSESSID")},
	{S_LIFETIME, 1440},
	{S_PROBABILITY, 1},
	{S_DIVISOR, 100},
	{C_PATH, string("/")},
	{C_LIFETIME, 0},
	{C_DOMAIN, string("")},
	{C_SECURE, false},
	{C_HTTPONLY, false}
});

static int set_tag(const char *path, const char *name, void *value, const size_t size) {
#if defined(__APPLE__)
	return setxattr(path, name, value, size, 0, 0);
#else
	return setxattr(path, name, value, size, 0);
#endif
}

static int get_tag(const char *path, const char *name, void *value, const size_t size) {
#if defined(__APPLE__)
	return getxattr(path, name, value, size, 0, 0);
#else
	return getxattr(path, name, value, size);
#endif
}

static void initialize_sparams(const array<mixed> &options) noexcept {
	for (const auto& it : skeys) {
		if (options.isset(string(it.first))) {
			set_sparam(it.first, options.get_value(string(it.first)));
			continue;
		}
		set_sparam(it.first, mixed(it.second));
	}
}

static array<mixed> session_get_cookie_params() {
	array<mixed> result;
	if (PhpScriptMutableGlobals::current().get_superglobals().v$_KPHPSESSARR.as_array().empty()) {
		php_warning("Session cookie params cannot be received when there is no active session. Returned the default params");
		result.emplace_value(string(C_PATH), skeys[6].second);
		result.emplace_value(string(C_LIFETIME), skeys[7].second);
		result.emplace_value(string(C_DOMAIN), skeys[8].second);
		result.emplace_value(string(C_SECURE), skeys[9].second);
		result.emplace_value(string(C_HTTPONLY), skeys[10].second);
	} else {
		result.emplace_value(string(C_PATH), get_sparam(C_PATH));
		result.emplace_value(string(C_LIFETIME), get_sparam(C_LIFETIME));
		result.emplace_value(string(C_DOMAIN), get_sparam(C_DOMAIN));
		result.emplace_value(string(C_SECURE), get_sparam(C_SECURE));
		result.emplace_value(string(C_HTTPONLY), get_sparam(C_HTTPONLY));
	}
	return result;
}

static void reset_sparams() noexcept {
	PhpScriptMutableGlobals::current().get_superglobals().v$_KPHPSESSARR.as_array().clear();
}

static mixed get_sparam(const char *key) noexcept {
	return get_sparam(string(key));
}

static mixed get_sparam(const string &key) noexcept {
	PhpScriptMutableGlobals &php_globals = PhpScriptMutableGlobals::current();
	if (!php_globals.get_superglobals().v$_KPHPSESSARR.as_array().isset(key)) {
		return false;
	}
	return php_globals.get_superglobals().v$_KPHPSESSARR.as_array().get_value(key);
}

static void set_sparam(const char *key, const mixed &value) noexcept {
	set_sparam(string(key), value);
}

static void set_sparam(const string &key, const mixed &value) noexcept {
	PhpScriptMutableGlobals::current().get_superglobals().v$_KPHPSESSARR.as_array().emplace_value(key, value);
}

static bool session_valid_id(const string &id) {
	if (id.empty()) {
		return false;
	}

	bool result = true;
	for (auto i = (string("sess_").size()); i < id.size(); ++i) {
		if (!((id[i] >= 'a' && id[i] <= 'z')
				|| (id[i] >= 'A' && id[i] <= 'Z')
				|| (id[i] >= '0' && id[i] <= '9')
				|| (id[i] == ',')
				|| (id[i] == '-'))) {
			result = false;
			break;
		}
	}
	return result;
}

static bool session_generate_id() {
	string id = f$uniqid(string("sess_"));
	if (!session_valid_id(id)) {
		php_warning("Failed to create new ID\n");
		return false;
	}
	set_sparam(S_ID, id);
	return true;
}

static bool session_abort() {
	if (get_sparam(S_STATUS).to_bool()) {
		session_close();
		return true;
	}
	return false;
}

static string session_encode() {
	return f$serialize(PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION.as_array());
}

static bool session_decode(const string &data) {
	mixed buf = f$unserialize(data);
	if (buf.is_bool()) {
		return false;
	}
	PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION = buf.as_array();
	return true;
}

static bool session_open() {
	if (get_sparam(S_FD).to_bool() && (fcntl(get_sparam(S_FD).to_int(), F_GETFD) != -1 || errno != EBADF)) {
		return true;
	}

	if (!f$file_exists(get_sparam(S_DIR).to_string())) {
		f$mkdir(get_sparam(S_DIR).to_string());
	}

	set_sparam(S_PATH, string(get_sparam(S_DIR).to_string()).append(get_sparam(S_ID).to_string()));
	bool is_new = (!f$file_exists(get_sparam(S_PATH).to_string())) ? 1 : 0;
	
	// the interprocessor lock does not work
	set_sparam(S_FD, open_safe(get_sparam(S_PATH).to_string().c_str(), O_RDWR | O_CREAT, 0666));

	if (get_sparam(S_FD).to_int() < 0) {
		php_warning("Failed to open the file %s", get_sparam(S_PATH).to_string().c_str());
		return false;
	}

	struct flock lock;
	lock.l_type = F_WRLCK; // Exclusive write lock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    int ret = fcntl(get_sparam(S_FD).to_int(), F_SETLKW, &lock);

    if (ret < 0 && errno == EDEADLK) {
    	php_warning("Attempt to lock alredy locked session file, path: %s", get_sparam(S_PATH).to_string().c_str());
    	return false;
    }

	// set new metadata to the file
	int ret_ctime = get_tag(get_sparam(S_PATH).to_string().c_str(), S_CTIME, NULL, 0);
	int ret_gc_lifetime = get_tag(get_sparam(S_PATH).to_string().c_str(), S_LIFETIME, NULL, 0);
	if (is_new or ret_ctime < 0) {
		// add the creation data to metadata of file
		int is_session = 1; // to filter sessions from other files in session_gc()
		set_tag(get_sparam(S_PATH).to_string().c_str(), "is_session", &is_session, sizeof(int));

		int ctime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		set_tag(get_sparam(S_PATH).to_string().c_str(), S_CTIME, &ctime, sizeof(int));
	}
	if (ret_gc_lifetime < 0) {
		int gc_maxlifetime = get_sparam(S_LIFETIME).to_int();
		set_tag(get_sparam(S_PATH).to_string().c_str(), S_LIFETIME, &gc_maxlifetime, sizeof(int));
	}

	return true;
}

static void session_close() {
	if (get_sparam(S_FD).to_bool() && (fcntl(get_sparam(S_FD).to_int(), F_GETFD) != -1 || errno != EBADF)) {
		struct flock lock;
		lock.l_type = F_UNLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_pid = getpid();
		fcntl(get_sparam(S_FD).to_int(), F_SETLKW, &lock);
		close_safe(get_sparam(S_FD).to_int());
	}
	set_sparam(S_STATUS, false);
	reset_sparams();
}

static bool session_read() {
	session_open();
	struct stat buf;
	if (fstat(get_sparam(S_FD).to_int(), &buf) < 0) {
		php_warning("Failed to read session data on path %s", get_sparam(S_PATH).to_string().c_str());
		return false;
	}

	if (buf.st_size == 0) {
		PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION.as_array().clear();
		return true;
	}

	char result[buf.st_size];
	ssize_t n = read_safe(get_sparam(S_FD).to_int(), result, buf.st_size, get_sparam(S_PATH).to_string());
	if (n < buf.st_size) {
		if (n == -1) {
			php_warning("Read failed");
		} else {
			php_warning("Read returned less bytes than requested");
		}
		return false;
	}
	
	if (!session_decode(string(result, n))) {
		php_warning("Failed to unzerialize the data");
		return false;
	}
	return true;
}

static bool session_write() {
	// rewind the S_FD of the session file
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_pid = getpid();
	fcntl(get_sparam(S_FD).to_int(), F_SETLKW, &lock);
	close_safe(get_sparam(S_FD).to_int());
	set_sparam(S_FD, open_safe(get_sparam(S_PATH).to_string().c_str(), O_RDWR, 0666));
	lock.l_type = F_WRLCK;
	fcntl(get_sparam(S_FD).to_int(), F_SETLKW, &lock);

	string data = f$serialize(PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION.as_array());
	ssize_t n = write_safe(get_sparam(S_FD).to_int(), data.c_str(), data.size(), get_sparam(S_PATH).to_string());
	if (n < data.size()) {
		if (n == -1) {
			php_warning("Write failed");
		} else {
			php_warning("Write wrote less bytes than requested");
		}
		return false;
	}
	return true;
}

static bool session_send_cookie() {
	if (strpbrk(get_sparam(S_NAME).to_string().c_str(), "=,;.[ \t\r\n\013\014") != NULL) {
		php_warning("session.name cannot contain any of the following '=,;.[ \\t\\r\\n\\013\\014'");
		return false;
	}

	int expire = get_sparam(C_LIFETIME).to_int();
	if (expire > 0) {
		expire += std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}
	string domain = get_sparam(C_DOMAIN).to_string();
	bool secure = get_sparam(C_SECURE).to_bool();
	bool httponly = get_sparam(C_HTTPONLY).to_bool();
	string sid = f$urlencode(get_sparam(S_ID).to_string());
	string path = get_sparam(C_PATH).to_string();
	string name = get_sparam(S_NAME).to_string();

	f$setcookie(name, sid, expire, path, domain, secure, httponly);
	return true;
}

static bool session_reset_id() {
	if (!get_sparam(S_ID).to_bool()) {
		php_warning("Cannot set session ID - session ID is not initialized");
		return false;
	}

	if (get_sparam(S_SEND_COOKIE).to_bool()) {
		session_send_cookie();
		set_sparam(S_SEND_COOKIE, false);
	}
	return true;
}

static bool session_expired(const string &path) {
	int ctime, lifetime;
	int ret_ctime = get_tag(path.c_str(), S_CTIME, &ctime, sizeof(int));
	int ret_lifetime = get_tag(path.c_str(), S_LIFETIME, &lifetime, sizeof(int));
	if (ret_ctime < 0 or ret_lifetime < 0) {
		php_warning("Failed to get metadata of the file on path: %s", path.c_str());
		return false;
	}

	int now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	if (ctime + lifetime <= now) {
		return true;
	}
	return false;
}

static int session_gc(const bool &immediate = false) {
	double prob = f$lcg_value() * get_sparam(S_DIVISOR).to_float();
	double s_prob = get_sparam(S_PROBABILITY).to_float();
	if ((!immediate) && ((s_prob <= 0) or (prob >= s_prob))) {
		return -1;
	}

	mixed s_list = f$scandir(get_sparam(S_DIR).to_string());
	if (!s_list.to_bool()) {
		php_warning("Failed to scan the session directory on the save_path: %s", get_sparam(S_DIR).to_string().c_str());
		return -1;
	}

	// reset the fd before changing the session directory
	close_safe(get_sparam(S_FD).to_int());

	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_pid = getpid();

	int result = 0;
	for (auto s = s_list.as_array().begin(); s != s_list.as_array().end(); ++s) {
		string path = s.get_value().to_string();
		if (path[0] == '.') {
			continue;
		}
		path = string(get_sparam(S_DIR).to_string()).append(path);
		if (path == get_sparam(S_PATH).to_string()) {
			continue;
		}

		{ // filter session files from others
			int is_session, ret_is_session = get_tag(path.c_str(), "is_session", &is_session, sizeof(int));
			if (ret_is_session < 0) {
				continue;
			}
		}
		
		int fd;
		if ((fd = open_safe(path.c_str(), O_RDWR, 0666)) < 0) {
			php_warning("Failed to open file on path: %s", path.c_str());
			continue;
		}

		if (fcntl(fd, F_SETLK, &lock) < 0) {
			continue;
		}

		if (session_expired(path)) {
			f$unlink(path);
			++result;
		}
	}

	lock.l_type = F_WRLCK;
	lock.l_pid = getpid();
	set_sparam(S_FD, open_safe(get_sparam(S_PATH).to_string().c_str(), O_RDWR, 0666));
	if (get_sparam(S_FD).to_int() < 0) {
		php_warning("Failed to reopen the file %s after session_gc()", get_sparam(S_PATH).to_string().c_str());
		session_abort();
	} else {
		fcntl(get_sparam(S_FD).to_int(), F_SETLKW, &lock);
	}

	return result;
}

static bool session_initialize() {
	set_sparam(S_STATUS, true);

	if (!get_sparam(S_ID).to_bool()) {
		if (!session_generate_id()) {
			php_warning(
				"Failed to create session ID: %s (path: %s)", 
				get_sparam(S_NAME).to_string().c_str(), 
				get_sparam(S_PATH).to_string().c_str()
			);
			session_abort();
			return false;
		}
		set_sparam(S_SEND_COOKIE, true);
	}

	if (!session_open() or !session_reset_id() or !session_read()) {
		session_abort();
		return false;
	}

	session_gc(0);

	return true;
}

static bool session_start() {
	if (get_sparam(S_STATUS).to_bool()) {
		php_warning("Ignoring session_start() because a session is already active");
		return false;
	}

	set_sparam(S_SEND_COOKIE, true);

	if (!get_sparam(S_ID).to_bool()) {
		mixed id = false;
		PhpScriptMutableGlobals &php_globals = PhpScriptMutableGlobals::current();
		if (php_globals.get_superglobals().v$_COOKIE.as_array().isset(get_sparam(S_NAME).to_string())) {
			id = php_globals.get_superglobals().v$_COOKIE.as_array().get_value(get_sparam(S_NAME).to_string()).to_string();
		}

		if (id.to_bool() && !id.to_string().empty()) {
			if (!strpbrk(id.to_string().c_str(), "\r\n\t <>'\"\\")) {
				if (f$file_exists(string(get_sparam(S_DIR).to_string()).append(id.to_string()))) {
					set_sparam(S_SEND_COOKIE, false);
					set_sparam(S_ID, id.to_string());
				}
			}
		}
	} else if (strpbrk(get_sparam(S_ID).to_string().c_str(), "\r\n\t <>'\"\\")) {
		set_sparam(S_ID, false);
	}

	if (!session_initialize()) {
		set_sparam(S_STATUS, false);
		set_sparam(S_ID, false);
		return false;
	}

	return true;
}

static bool session_flush() {
	if (!get_sparam(S_STATUS).to_bool()) {
		return false;
	}

	session_write();
	session_close();
	return true;
}

} // namespace sessions

bool f$session_start(const array<mixed> &options) {
	if (sessions::get_sparam(sessions::S_STATUS).to_bool()) {
		php_warning("Ignoring session_start() because a session is already active");
		return false;
	}

	sessions::initialize_sparams(options);
	sessions::session_start();

	if (sessions::get_sparam(sessions::S_READ_CLOSE).to_bool()) {
		sessions::session_close();
	}
	return true;
}

bool f$session_abort() {
	if (!sessions::get_sparam(sessions::S_STATUS).to_bool()) {
		return false;
	}
	sessions::session_abort();
	return true;
}

Optional<int64_t> f$session_gc() {
	if (!sessions::get_sparam(sessions::S_STATUS).to_bool()) {
		php_warning("Session cannot be garbage collected when there is no active session");
		return false;
	}
	int result = sessions::session_gc(1);
	return (result <= 0) ? Optional<int64_t>{false} : Optional<int64_t>{result};
}

bool f$session_write_close() {
	if (!sessions::get_sparam(sessions::S_STATUS).to_bool()) {
		return false;
	}
	sessions::session_flush();
	return true;
}

bool f$session_commit() {
	return f$session_write_close();
}

int64_t f$session_status() {
	return sessions::get_sparam(sessions::S_STATUS).to_int() + 1;
}

Optional<string> f$session_encode() {
	return Optional<string>{sessions::session_encode()};
}

bool f$session_decode(const string &data) {
	if (!sessions::get_sparam(sessions::S_STATUS).to_bool()) {
		php_warning("Session data cannot be decoded when there is no active session");
		return false;
	}
	return sessions::session_decode(data);
}

array<mixed> f$session_get_cookie_params() {
	return sessions::session_get_cookie_params();
}

Optional<string> f$session_id(const Optional<string> &id) {
	if (id.has_value() && sessions::get_sparam(sessions::S_STATUS).to_bool()) {
		php_warning("Session ID cannot be changed when a session is active");
		return Optional<string>{false};
	}
	mixed prev_id = sessions::get_sparam(sessions::S_ID);
	if (id.has_value()) {
		sessions::set_sparam(sessions::S_ID, id.val());
	}
	return (prev_id.is_bool()) ? Optional<string>{false} : Optional<string>(prev_id.as_string());
}

// TO-DO
// bool f$session_destroy() {
// 	if (!sessions::get_sparam(sessions::S_STATUS).to_bool()) {
// 		php_warning("Trying to destroy uninitialized session");
// 		return false;
// 	}
// 	sessions::session_close();
// 	return true;
// }
