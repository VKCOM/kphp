#pragma once

#include "runtime/kphp_core.h"
#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

namespace sessions {

using Stream = mixed;

class SessionManager : vk::not_copyable {
public:
	bool session_open();
	bool session_close(bool immediate = false);
	bool session_read();
	bool session_write();
	bool session_delete();

	void session_reset_vars();

	void set_session_status(const bool &status) noexcept { session_status = status; }
	void set_session_path(const string &path) noexcept { session_path = path; }
	void set_session_id(const Optional<string> &sid) noexcept { id = sid; }
	void set_send_cookie(const bool &val) noexcept { send_cookie = val; }
	void set_session_name(const string &val) noexcept { session_name = val; }
	void set_session_lifetime(const int &val) noexcept { session_lifetime = val; }
	void set_handler(const Stream &stream) noexcept { handler = stream; }

	void set_cookie_domain(const string &val) noexcept { cookie_domain = val; }
	void set_cookie_lifetime(const int &val) noexcept { cookie_lifetime = val; }
	void set_cookie_secure(const bool &val) noexcept { cookie_secure = val; }
	void set_cookie_httponly(const bool &val) noexcept { cookie_httponly = val; }
	
	bool get_session_status() noexcept { return session_status; }
	Optional<string> get_session_path() noexcept { return session_path; }
	Optional<string> get_session_id() noexcept { return id; }
	bool get_send_cookie() noexcept { return send_cookie; }
	Optional<string> get_session_name() noexcept { return session_name; }
	int get_session_lifetime() noexcept { return session_lifetime; }
	Stream get_handler() noexcept {return handler; }
	
	string get_sessions_path() noexcept { return sessions_path; }
	Optional<string> get_cookie_domain() noexcept { return cookie_domain; }
	int get_cookie_lifetime() noexcept { return cookie_lifetime; }
	bool get_cookie_secure() noexcept { return cookie_secure; }
	bool get_cookie_httponly() noexcept { return cookie_httponly; }

private:
	SessionManager();

	Optional<string> id{false};
	bool session_status{0};
	bool send_cookie{0};
	Optional<string> session_path{false};
	Optional<string> session_name{false}; // by default PHPSESSID (key for searching in the cookies)
	int session_lifetime{1440};

	int cookie_lifetime{0};
	Optional<string> cookie_domain{false};
	bool cookie_secure{0};
	bool cookie_httponly{0};

	string tempdir_path; 	// path to the /tmp
	string sessions_path;	// path to the dir with existing sessions

	Stream handler{NULL};

	friend class vk::singleton<SessionManager>;
};

} // namespace sessions

bool f$session_start(const array<mixed> &options = array<mixed>());
bool f$session_abort();
bool f$session_destroy();
bool f$session_commit();
bool f$session_write_close();
int64_t f$session_status();
Optional<string> f$session_create_id(const string &prefix);
