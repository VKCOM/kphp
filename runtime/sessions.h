#pragma once

#include "runtime/kphp_core.h"
#include "runtime/streams.h"

namespace sessions {

class SessionManager : vk::not_copyable {
public:
	bool session_open();
	bool session_close();
	bool session_read();
	bool session_write();
	bool session_delete();

	const bool &set_session_status(const bool &status) const noexcept { return session_status = status; }
	const bool &get_session_status() const noexcept { return session_status; }
	const Optional<string> &set_session_path(const bool &path) const noexcept { return session_path = path; }
	const Optional<string> &get_session_path() const noexcept { return session_path; }
	const Optional<string> &set_session_id(const string &sid) const noexcept { return id = sid; }
	const Optional<string> &get_session_id() const noexcept { return id; }
	const Optional<string> &set_send_cookie(const bool &val) const noexcept { return send_cookie = val; }
	const Optional<string> &get_send_cookie() const noexcept { return send_cookie; }
	const Optional<string> &set_session_name(const string &val) const noexcept { return session_name = val; }
	const Optional<string> &get_session_name() const noexcept { return session_name; }

	const Optional<Stream> &set_handler(const Stream &stream) const noexcept {return handler = stream; }
	const Optional<Stream> &get_handler() const noexcept {return handler; }

private:
	SessionManager() = default;

	Optional<string> id{NULL};
	bool session_status{0};
	bool send_cookie{0};
	Optional<string> session_path{NULL};
	Optional<string> session_name{NULL}; // by default PHPSESSID (key for searching in the cookies)

	const string tempdir_path; 	// path to the /tmp
	const string sessions_path;	// path to the dir with existing sessions

	Optional<Stream> handler{NULL};

	friend class vk::singleton<SessionManager>;
};

} // namespace sessions

bool f$session_start();
bool f$session_abort();
bool f$session_commit();
bool f$session_write_close();
int64_t f$session_status();
Optional<string> f$session_create_id(const string &prefix);
