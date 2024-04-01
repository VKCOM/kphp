#include <chrono>
#include <sys/stat.h>

#include "runtime/sessions.h"
#include "runtime/files.h"
#include "runtime/serialize-functions.h"
#include "runtime/misc.h"
#include "common/wrappers/to_array.h"
#include "runtime/interface.h"
#include "runtime/url.h"

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
// static bool session_flush();

static bool session_open();
static bool session_read();
// static bool session_write();
static void session_close();

static void initialize_sparams(const array<mixed> &options) noexcept {
	// TO-DO: reconsider it
	const auto skeys = vk::to_array<std::pair<const char *, mixed>>({
		{"read_and_close", false},
		{"save_path", string(getenv("TMPDIR")).append("sessions/")},
		{"name", string("PHPSESSID")},
		{"gc_maxlifetime", 1440},
		{"cookie_path", string("/")},
		{"cookie_lifetime", 0},
		{"cookie_domain", string("")},
		{"cookie_secure", false},
		{"cookie_httponly", false}
	});

	reset_sparams();

	for (const auto& it : skeys) {
		if (options.isset(string(it.first))) {
			set_sparam(it.first, options.get_value(string(it.first)));
			continue;
		}
		set_sparam(it.first, mixed(it.second));
	}
}

static void print_sparams() noexcept {
	fprintf(stdout, "\nKPHPSESSARR is:\n");
	for (auto it = v$_KPHPSESSARR.as_array().begin(); it != v$_KPHPSESSARR.as_array().end(); ++it) {
		fprintf(stdout, "%s : %s\n", it.get_key().to_string().c_str(), it.get_value().to_string().c_str());
	}

	fprintf(stdout, "\n%s\n", f$serialize(v$_KPHPSESSARR.as_array()).c_str());
}

static void reset_sparams() noexcept {
	v$_KPHPSESSARR = array<mixed>();
}

static mixed get_sparam(const char *key) noexcept {
	return get_sparam(string(key));
}

static mixed get_sparam(const string &key) noexcept {
	if (!v$_KPHPSESSARR.as_array().isset(key)) {
		return false;
	}
	return v$_KPHPSESSARR.as_array().get_value(key);
}

static void set_sparam(const char *key, const mixed &value) noexcept {
	set_sparam(string(key), value);
}

static void set_sparam(const string &key, const mixed &value) noexcept {
	v$_KPHPSESSARR.as_array().emplace_value(key, value);
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
	set_sparam("session_id", id);
	return true;
}

static bool session_abort() {
	if (get_sparam("session_status").to_bool()) {
		session_close();
		set_sparam("session_status", false);
		return true;
	}
	return false;
}

static bool session_open() {
	if (get_sparam("handler").to_bool()) {
		/* 
			if we close the file for reopening it, 
			the other worker may open it faster and the current process will stop
		*/
		return true;
	}

	if (!f$file_exists(get_sparam("save_path").to_string())) {
		f$mkdir(get_sparam("save_path").to_string());
		fprintf(stdout, "session_open(): successfully created dir: %s\n", get_sparam("save_path").to_string().c_str());
	}

	set_sparam("file_path", string(get_sparam("save_path").to_string()).append(get_sparam("session_id").to_string()));
	set_sparam("handler", open_safe(get_sparam("file_path").to_string().c_str(), O_RDWR | O_CREAT, 0777));

	if (get_sparam("handler").to_int() < 0) {
		php_warning("Failed to open the file %s", get_sparam("file_path").to_string().c_str());
		return false;
	}
	fprintf(stdout, "session_open(): successfully opened file: handler %lld, path %s\n", get_sparam("handler").to_int(), get_sparam("file_path").to_string().c_str());

	if (flock(get_sparam("handler").to_int(), LOCK_EX) < 0) {
		php_warning("Failed to lock the file %s", get_sparam("file_path").to_string().c_str());
		return false;
	}
	
	fprintf(stdout, "session_open(): successfully locked file: handler %lld, path %s\n", get_sparam("handler").to_int(), get_sparam("file_path").to_string().c_str());

	return true;
}

static void session_close() {
	if (get_sparam("handler").to_bool()) {
		close_safe(get_sparam("handler").to_int());
	}
	reset_sparams();
}

static bool session_read() {
	session_open();
	struct stat buf;
	if (fstat(get_sparam("handler").to_int(), &buf) < 0) {
		php_warning("Failed to read session data on path %s", get_sparam("file_path").to_string().c_str());
		return false;
	}

	if (buf.st_size == 0) {
		v$_SESSION = array<mixed>();
		return true;
	}

	char result[buf.st_size];
	dl::enter_critical_section();
	int n = read(get_sparam("handler").to_int(), result, buf.st_size);
	if (n < (int)buf.st_size) {
		if (n == -1) {
			php_warning("Read failed");
		} else {
			php_warning("Read returned less bytes than requested");
		}
		dl::leave_critical_section();
		return false;
	}
	dl::leave_critical_section();

	v$_SESSION = f$unserialize(string(result, n));
	fprintf(stdout, "session_open(): successfully read file: handler %lld, path %s\n", get_sparam("handler").to_int(), get_sparam("file_path").to_string().c_str());
	fprintf(stdout, "_SESSION is: \n");
	for (auto it = v$_SESSION.as_array().begin(); it != v$_SESSION.as_array().end(); ++it) {
		fprintf(stdout, "%s : %s\n", it.get_key().to_string().c_str(), it.get_value().to_string().c_str());
	}
	return true;
}

// static bool session_write() {
// 	session_open();
// 	// pass
// 	return true;
// }

static bool session_send_cookie() {
	if (strpbrk(get_sparam("name").to_string().c_str(), "=,;.[ \t\r\n\013\014") != NULL) {
		php_warning("session.name cannot contain any of the following '=,;.[ \\t\\r\\n\\013\\014'");
		return false;
	}

	int expire = get_sparam("cookie_lifetime").to_int();
	if (expire > 0) {
		expire += std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}
	string domain = get_sparam("cookie_domain").to_string();
	bool secure = get_sparam("cookie_secure").to_bool();
	bool httponly = get_sparam("cookie_httponly").to_bool();
	string sid = f$urlencode(get_sparam("session_id").to_string());
	string path = get_sparam("cookie_path").to_string();
	string name = get_sparam("name").to_string();

	f$setcookie(name, sid, expire, path, domain, secure, httponly);
	return true;
}

static bool session_reset_id() {
	if (!get_sparam("session_id").to_bool()) {
		php_warning("Cannot set session ID - session ID is not initialized");
		return false;
	}

	if (get_sparam("send_cookie").to_bool()) {
		session_send_cookie();
		set_sparam("send_cookie", true);
	}
	return true;
}

static bool session_initialize() {
	set_sparam("session_status", true);

	if (!get_sparam("session_id").to_bool()) {
		if (!session_generate_id()) {
			php_warning(
				"Failed to create session ID: %s (path: %s)", 
				get_sparam("name").to_string().c_str(), 
				get_sparam("save_path").to_string().c_str()
			);
			session_abort();
			return false;
		}
		set_sparam("send_cookie", true);
	}
	fprintf(stdout, "session_initialize(): successfully generated id: %s\n", get_sparam("session_id").to_string().c_str());

	if (!session_open() or !session_reset_id() or !session_read()) {
		session_abort();
		return false;
	}

	// session_gc()
	// pass

	return true;
}

static bool session_start() {
	if (get_sparam("session_status").to_bool()) {
		php_warning("Ignoring session_start() because a session is already active");
		return false;
	}

	set_sparam("send_cookie", true);

	if (!get_sparam("session_id").to_bool()) {
		mixed id = false;
		if (v$_COOKIE.as_array().isset(get_sparam("name").to_string())) {
			id = v$_COOKIE.as_array().get_value(get_sparam("name").to_string()).to_string();
			fprintf(stdout, "Found id: %s\n", id.as_string().c_str());
		}

		if (id.to_bool() && !id.to_string().empty()) {
			if (!strpbrk(id.to_string().c_str(), "\r\n\t <>'\"\\")) {
				fprintf(stdout, "id is valid\n");
				set_sparam("send_cookie", false);
				set_sparam("session_id", id.to_string());
			}
		}
	}

	if (!session_initialize()) {
		set_sparam("session_status", false);
		set_sparam("session_id", false);
		return false;
	}

	return true;
}

// static bool session_flush() {
// 	if (!get_sparam("session_status").to_bool()) {
// 		return false;
// 	}

// 	session_write();
// 	set_sparam("session_status", false);
// 	return true;
// }

} // namespace sessions

bool f$session_start(const array<mixed> &options) {
	if (sessions::get_sparam("session_status").to_bool()) {
		php_warning("Ignoring session_start() because a session is already active");
		return false;
	}

	sessions::initialize_sparams(options);
	sessions::print_sparams(); // to delete
	sessions::session_start();

	if (sessions::get_sparam("read_and_close").to_bool()) {
		sessions::session_close();
	}
	return true;
}

bool f$session_abort() {
	if (!sessions::get_sparam("session_status").to_bool()) {
		return false;
	}
	sessions::session_abort();
	return true;
}
