#include <chrono>
#include <sys/stat.h>

#include "runtime/sessions.h"
#include "runtime/serialize-functions.h"
#include "runtime/interface.h"
#include "runtime/streams.h"
#include "runtime/url.h"
#include "runtime/files.h"
#include "runtime/misc.h"
// #include "runtime/critical_section.h"

namespace sessions {

SessionManager::SessionManager() {
	tempdir_path = string(getenv("TMPDIR"));
	sessions_path = string(tempdir_path).append("sessions/");
	if (!f$file_exists(sessions_path)) {
		f$mkdir(sessions_path);
	}
}

static SessionManager &SM = vk::singleton<SessionManager>::get();

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

static bool generate_session_id() {
	if (!SM.get_session_status()) {
		return false;
	}
	string id = f$uniqid(string("sess_"));

	if (!session_valid_id(id)) {
		php_warning("Failed to create new ID\n");
		return false;
	}
	SM.set_session_id(id);
	return true;
}

void SessionManager::session_reset_vars() {
	handler = NULL;
	id = false;
	session_status = send_cookie = 0;
	cookie_secure = cookie_httponly = 0;
	cookie_lifetime = 0;
	session_path = session_name = cookie_domain = false;
}

bool SessionManager::session_write() {
	if (!session_open()) {
		return false;
	}
	f$file_put_contents(handler.to_string(), string("")); // f$ftruncate(handler, 0);
	f$rewind(handler);
	{ // additional local scope for temprory variables tdata and tres
		string tdata = f$serialize(v$_SESSION);
		mixed tres = f$file_put_contents(handler.to_string(), tdata);
		if (tres.is_bool() || static_cast<size_t>(tres.as_int()) != tdata.size()) {
			// php_warning();
			return false;
		}
	}
	return true;
}

bool SessionManager::session_read() {
	if ((handler.is_null()) || (!session_open())) {	
		// php_warning();
		return false;
	}
	{ // additional local scope for temprory variable tdata
		Optional<string> tdata;
		tdata = f$file_get_contents(handler.to_string());
		if (tdata.is_false()) {
			return false;
		}
		v$_SESSION = f$unserialize(tdata.val()); // TO-DO: check
	}
	fprintf(stdout, "successfully read the session data with id %s\n", SM.get_session_id().val().c_str());
	return true;
}

bool SessionManager::session_open() {
	if (id.is_false()) {
		return false;
	}
	if (!handler.is_null()) {
		session_close();
	}

	if (session_path.is_false()) {
		// setting the session_path is only here
		session_path = sessions_path;
		session_path.val().append(id.val());
		session_path.val().append(".sess");
	}
	
	if (!f$file_exists(session_path.val())) {
		f$fclose(f$fopen(session_path.val(), string("w+")));
	}

	{ // additional local scope for temprory variable thandler
		mixed thandler = f$fopen(session_path.val(), string("r+"));
		handler = (!thandler.is_bool()) ? thandler : NULL;
	}
	if (handler.is_null()) {
		return false;
	}
	return true;
}

static bool session_destroy(const string &filepath) {
	
	Optional<int64_t> ctime = f$filectime(filepath);
	if (ctime.is_false()) {
		php_warning("Failed to get metadata of file %s\n", filepath.c_str());
	}

	// Stream thandler = f$fopen(filepath, string("r"));
	// if (!thandler) {
	// 	php_warning("Failed to open file %s\n", filepath.c_str());
	// 	return false;
	// }

	Optional<string> sdata = f$file_get_contents(filepath);
	if (sdata.is_false()) {
		php_warning("Failed to read file %s\n", filepath.c_str());
		return false;
	}
	array<mixed> tdata = f$unserialize(sdata.val()).to_array();
	int lifetime = 0;
	if (tdata.has_key(string("gc_maxlifetime"))) {
		lifetime = tdata.get_value(string("gc_maxlifetime")).to_int();
	}
	if (ctime < lifetime) {
		// f$fclose(thandler);
		return false;
	}
	// f$fclose(thandler);
	return f$unlink(filepath);
}

static bool session_gc() {
	Optional<array<string>> sfiles = f$scandir(SM.get_sessions_path());
	if (sfiles.is_false()) {
		php_warning("Failed to scan sessions directory %s\n", SM.get_sessions_path().c_str());
		return false;
	}
	fprintf(stdout, "result of scandir is:\n%s\n", f$serialize(sfiles.val()).c_str());
	auto nsfiles = sfiles.val().count(); 
	int n = 0;
	for (auto sfile = sfiles.val().cbegin(); sfile != sfiles.val().cend(); ++sfile) {
		string val = sfile.get_value().to_string();
		if (val == string(".") or val == string("..")) {
			continue;
		}
		if (session_destroy(val)) {
			++n;
		}
	}
	if (n != nsfiles) {
		php_warning("Failed to delete all files in sessions dir %s\n", SM.get_sessions_path().c_str());
		return false;
	}
	return true;
}

bool SessionManager::session_close(bool immediate) {
	if (handler.is_null()) {
		return true;
	}
	
	if (!f$fclose(handler)) {
		// php_warning();
		return false;
	}
	session_reset_vars();

	if (immediate or rand() % 100 > 75) {
		session_gc();
	}
	return true;
}

static bool session_abort() {
	if (SM.get_session_status()) {
		SM.session_close();
		SM.set_session_status(0);
		return true;
	}
	return false;
}

static bool session_flush(bool write = false) {
	if (!SM.get_session_status()) {
		return false;
	}
	bool result = true;
	if (write && !SM.session_write()) {
		php_warning("Failed to write session data %s\n", SM.get_session_path().val().c_str());
		result = false;
	}

	SM.set_session_status(0);
	SM.session_close();
	return result;
}

static bool send_cookies() {
	// 1. check whether headers were sent or not
	// pass: need headers_sent()

	// 2. check session_name for deprecated symbols, if it's user supplied
	if (strpbrk(SM.get_session_name().val().c_str(), "=,;.[ \t\r\n\013\014") != NULL) {
		php_warning("session.name cannot contain any of the following '=,;.[ \\t\\r\\n\\013\\014'");
		return false;
	}

	// 3. encode session_name, if it's user supplied
	int expire = SM.get_session_lifetime();
	if (expire > 0) {
		expire += std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}
	string domain = !SM.get_cookie_domain().is_false() ? SM.get_cookie_domain().val() : string("");
	f$setcookie(SM.get_session_name().val(), f$urlencode(SM.get_session_id().val()), expire, string("/"), domain, SM.get_cookie_secure(), SM.get_cookie_httponly());
	fprintf(stdout, "successfully sent cookies for id %s\n", SM.get_session_id().val().c_str());
	return true;
}

static bool session_reset_id() {
	if (SM.get_session_id().is_false()) {
		php_warning("Cannot set session ID - session ID is not initialized");
		return false;
	}

	if (SM.get_send_cookie()) {
		send_cookies();
		SM.set_send_cookie(0);
	}

	return true;
}

static bool session_initialize() {
	SM.set_session_status(1);

	if (SM.get_session_id().is_false()) {
		generate_session_id();
		if (SM.get_session_id().is_false()) {
			session_abort();
			php_warning("Failed to create session ID: %s (path: %s)", SM.get_session_name().val().c_str(), SM.get_session_path().val().c_str());
			return false;
		}
		SM.set_send_cookie(1);
	}

	SM.session_open();
	if (SM.get_handler().is_null()) {
		session_abort();
		// php_warning();
		return false;
	}
	
	session_reset_id();

	if (!SM.session_read()) {
		session_abort();
		php_warning("Failed to read session data: %s (path: %s)\n", SM.get_session_name().val().c_str(), SM.get_session_path().val().c_str());
		return false;
	}

	return true;
}

static bool session_start() {
	// 1. check status of the session
	if (SM.get_session_status()) {
		if (SM.get_session_path().is_false()) {
			php_warning("Ignoring session_start() because a session is already active (started from %s)", SM.get_session_path().val().c_str());
		} else {
			php_warning("Ignoring session_start() because a session is already active");
		}
		return false;
	}

	SM.set_send_cookie(1);

	if (SM.get_session_name().is_false()) {
		SM.set_session_name(string("PHPSESSID"));
	}
	fprintf(stdout, "session name is: %s\n", SM.get_session_name().val().c_str());

	// 2. check id of session
	if (SM.get_session_id().is_false()) {
		// 4.1 try to get an id of session via it's name from _COOKIE
		Optional<string> pid{false}; 
		if (v$_COOKIE.as_array().has_key(SM.get_session_name().val())) {
			pid = v$_COOKIE.get_value(SM.get_session_name().val()).to_string();
			fprintf(stdout, "found session id in cookies: %s\n", pid.val().c_str());
		}
		if (!pid.is_false()) {
			SM.set_send_cookie(0);
		}
		SM.set_session_id(pid);
	}

	// 5. check the id for dangerous symbols
	if (!SM.get_session_id().is_false() && strpbrk(SM.get_session_id().val().c_str(), "\r\n\t <>'\"\\")) {
		SM.set_session_id(false);
	}

	// 6. try to initialize the session
	if (!session_initialize()) {
		SM.set_session_status(0);
		SM.set_session_id(false);
		return false;
	}
	return true;
}

} // namespace sessions

bool f$session_start(const array<mixed> &options) {
	// 1. check status of session
	if (sessions::SM.get_session_status()) {
		if (sessions::SM.get_session_path().is_false()) {
			php_warning("Ignoring session_start() because a session is already active (started from %s)", sessions::SM.get_session_path().val().c_str());
		} else {
			php_warning("Ignoring session_start() because a session is already active");
		}
		return true;
	}

	// 2. check whether headers were sent or not
	// need headers_sent()
	// pass

	bool read_and_close = false;

	// 3. parse the options arg and set it
	if (!options.empty()) {
		for (auto it = options.begin(); it != options.end(); ++it) {
			string tkey = it.get_key().to_string();
			if (tkey == string("read_and_close")) {
				read_and_close = it.get_value().to_bool();
			} else if (tkey == string("name")) {
				sessions::SM.set_session_name(it.get_value().to_string());
			} else if (tkey == string("gc_maxlifetime")) {
				sessions::SM.set_session_lifetime(it.get_value().to_int());
			} else if (tkey == string("cookie_path")) {
				sessions::SM.set_session_path(it.get_value().to_string());
			} else if (tkey == string("cookie_lifetime")) {
				sessions::SM.set_cookie_lifetime(it.get_value().to_int());
			} else if (tkey == string("cookie_domain")) {
				sessions::SM.set_cookie_domain(it.get_value().to_string());
			} else if (tkey == string("cookie_secure")) {
				sessions::SM.set_cookie_secure(it.get_value().to_bool());
			} else if (tkey == string("cookie_httponly")) {
				sessions::SM.set_cookie_httponly(it.get_value().to_bool());
			}
		}
	}
	fprintf(stdout, "Before session_start()\n");
	sessions::session_start();
	fprintf(stdout, "After session_start()\n");

	// 4. check the status of the session
	if (!sessions::SM.get_session_status()) {
		// 4.1 clear cache of session variables
		v$_SESSION = NULL;
		// php_warning();
		return false;
	}

	// 5. check for read_and_close
	if (read_and_close) {
		sessions::session_flush();
	}

	return true;
}

bool f$session_destroy() {
	if (!sessions::SM.get_session_status()) {
		php_warning("Trying to destroy uninitialized session");
		return false;
	}

	if (sessions::SM.get_session_path().has_value()) {
		return sessions::session_destroy(sessions::SM.get_session_path().val());
	}
	sessions::SM.session_close();
	return false;
}

bool f$session_abort() {
	if (!sessions::SM.get_session_status()) {
		return false;
	}
	return sessions::session_abort();
}

int64_t f$session_status() {
	return static_cast<int64_t>(sessions::SM.get_session_status()) + 1;
}

bool f$session_write_close() {
	if (!sessions::SM.get_session_status()) {
		return false;
	}
	return sessions::session_flush(1);
}

bool $session_commit() {
	return f$session_write_close();
}

