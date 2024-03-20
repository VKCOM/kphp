#include "runtime/sessions.h"
#include "runtime/serialize-functions.h"
#include "runtime/interface.h"
// #include "runtime/files.h"
// #include "runtime/critical_section.h"


/* 
TO-DO:
- generate_session_id()
- send_cookies()
- session_start(): check the id for dangerous symbols
- f$session_start():
	- check whether headers were sent or not
	- parse argument const array<mixed> options
	- read_and_close
	- session_flush()
- f$session_destroy()
*/

namespace sessions {

static const SM = vk::singleton<SessionManager>::get();

SM.tempdir_path = string(getenv("TMPDIR"));
SM.sessions_path = SM.tempdir_path + string("/sessions/");
if (!f$file_exists(SM.sessions_path)) {
	f$mkdir(SM.sessions_path);
}

static bool generate_session_id() {
	// pass
}

bool SessionManager::session_write() {
	if (!session_open()) {
		return false;
	}
	f$file_put_contents(handler, string("")); // f$ftruncate(handler, 0);
	f$rewind(handler);
	{ // additional local scope for temprory variables tdata and tres
		string tdata = f$serialize(v$_SESSION);
		mixed tres = f$file_put_contents(handler, tdata);
		if (tres.is_bool() || static_cast<size_t>(tres.as_int()) != tdata.size()) {
			php_warning();
			return false;
		}
	}
	return true;
}

bool SessionManager::session_read() {
	if ((handler.is_null()) || (!session_open())) {	
		php_warning();
		return false;
	}
	{ // additional local scope for temprory variable tdata
		Optional<string> tdata;
		data = f$file_get_contents(handler);
		if (data.is_false() or data.is_null()) {
			return false;
		}
		v$_SESSION = f$unserialize(data.val());
	}
	return true;
}

bool SessionManager::session_open() {
	if (id.is_null()) {
		return false;
	}
	if (!handler.is_null()) {
		session_close();
	}

	if (session_path.is_null()) {
		// setting the session_path is only here => can be unchecked if it exists
		session_path = sessions_path + id.val() + string("/");
		if (!f$is_dir(session_path)) {
			f$mkdir(session_path);
		}
		session_path += id.val() + string(".sess");
	}
	
	{ // additional local scope for temprory variable thandler
		mixed thandler = f$fopen(session_path, string("r+"));
		handler = (!thandler.is_bool()) ? thandler : NULL;
	}
	if (handler.is_null()) {
		return false;
	}
	return true;
}

bool SessionManager::session_close() {
	if (handler.is_null()) {
		return true;
	}
	
	if (!f$fclose(handler)) {
		php_warning();
		return false;
	}
	handler = NULL;
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

static bool session_flush() {
	// pass
}

static bool send_cookies() {
	// 1. check whether headers were sent or not
	// pass: can return false here

	// 2. check session_name for deprecated symbols, if it's user supplied
	// pass: can return false here

	// 3. encode session_name, if it's user supplied
	// pass: can return false here

	// 4. set specific headers in the cookies
	f$setcookie(/* some data here */);
	return true;
}

static bool session_reset_id() {
	if (SM.get_session_id().is_null()) {
		php_warning("Cannot set session ID - session ID is not initialized");
		return false;
	}

	if (SM.get_send_cookie()) {
		send_cookies(); // TO-DO
		SM.set_send_cookie(0);
	}

	return true;
}

static bool session_initialize() {
	SM.set_session_status(1);

	SM.session_open();
	if (SM.get_handler().is_null()) {
		session_abort();
		php_warning();
		return false;
	}

	if (SM.get_session_id().is_null()) {
		SM.generate_session_id(); // TO-DO
		if (SM.get_session_id().is_null()) {
			session_abort();
			php_warning("Failed to create session ID: %s (path: %s)", SM.session_name.c_str(), SM.get_session_path().val().c_str());
			return false;
		}
		SM.set_send_cookie(1);
	}
	
	session_reset_id();

	if (!SM.session_read()) {
		SM.session_abort();
		php_warning("Failed to read session data: %s (path: %s)", SM.session_name.c_str(), SM.get_session_path().val().c_str());
		return false;
	}
	return true;
}

static void session_start() {
	// 1. check status of the session
	if (SM.get_session_status()) {
		if (SM.get_session_path().has_value()) {
			php_warning("Ignoring session_start() because a session is already active (started from %s)", SM.get_session_path().val().c_str());
		} else {
			php_warning("Ignoring session_start() because a session is already active");
		}
		return false;
	}

	SM.set_send_cookie(1);

	if (SM.get_session_name().is_null()) {
		SM.set_session_name("PHPSESSID"); // or "KPHPSESSID"
	}

	// 2. check id of session
	if (!SM.get_session_id().is_null()) {
		// 4.1 try to get an id of session via it's name from _COOKIE
		mixed pid = (v$_COOKIE.has_key(SM.session_name)) ? v$_COOKIE.get_value(SM.session_name) : NULL;
		if (!pid.is_string()) {
			pid = NULL;
		} else {
			SM.set_send_cookie(0);
		}
		SM.set_session_id(pid);
	}

	// 5. check the id for dangerous symbols
	// pass

	// 6. try to initialize the session
	if (!session_initialize()) {
		SM.set_session_status(0);
		SM.set_session_id(NULL);
		return false;
	}
	return true;
}

} // namespace sessions

bool f$session_start(/*const array<mixed> &options*/) {
	// 1. check status of session
	if (sessions::SM.get_session_status()) {
		if (sessions::SM.get_session_path().has_value()) {
			php_warning("Ignoring session_start() because a session is already active (started from %s)", sessions::SM.get_session_path().val().c_str());
		} else {
			php_warning("Ignoring session_start() because a session is already active");
		}
		return true;
	}

	// 2. check whether headers were sent or not
	// for this kphp must have the headers_sent() function

	// bool read_and_close = false;

	// 3. parse the options arg and set it
	// if (!options.empty()) {
	// 	for (auto it = options.begin(); it != options.end(); ++it) {
	// 		if (it.get_key().to_string() == "read_and_close") {
	// 			read_and_close = true;
	// 		} else {
	// 			// 3.1 get the value and modify the session header with the same key
	// 			// store to the buffer data
	// 			// pass
	// 		}
	// 	}
	// }

	sessions::session_start();

	// 4. check the status of the session
	if (!sessions::MS.get_session_status()) {
		// 4.1 clear cache of session variables
		// pass
		php_warning();
		return false;
	}

	// 5. check for read_and_close
	// if (read_and_close) {
	// 	// session_flush()
	// 	// pass
	// }

	return true;
}

