#include <gtest/gtest.h>
#include <sys/stat.h>

#include "runtime/files.h"
#include "runtime/sessions.h"
#include "runtime/php-script-globals.h"
#include "runtime/serialize-functions.h"
#include "runtime/exec.h"

constexpr int SESSION_NONE = 1;
constexpr int SESSION_ACTIVE = 2;

const auto tmpdir_path = string(getenv("TMPDIR") != nullptr ? getenv("TMPDIR") : "/tmp/");
const auto session_dir_path = string(tmpdir_path).append("sessions/");
const auto example_dir_path = string(tmpdir_path).append("example/");

TEST(sessions_test, test_session_id_with_invalid_id) {
	const string id = string("\t12345678\\\r");
	ASSERT_FALSE(f$session_id(id).has_value());
	ASSERT_TRUE(f$session_start());
	ASSERT_NE(f$session_id().val(), id);

	const string full_path = string(session_dir_path).append(f$session_id().val());
	ASSERT_TRUE(f$file_exists(full_path));
	ASSERT_TRUE(f$session_abort());

	f$exec(string("rm -rf ").append(session_dir_path));
}

TEST(sessions_test, test_session_id_with_valid_id) {
	const string id = string("sess_668d4f818ca3b");
	ASSERT_FALSE(f$session_id(id).has_value());
	ASSERT_TRUE(f$session_start());
	ASSERT_TRUE(f$session_id().has_value() && !f$session_id().val().empty());
	ASSERT_EQ(f$session_id().val(), id);

	const string full_path = string(session_dir_path).append(id);
	ASSERT_TRUE(f$file_exists(full_path));
	ASSERT_TRUE(f$session_abort());

	f$exec(string("rm -rf ").append(session_dir_path));
}

TEST(sessions_test, test_session_start) {
	ASSERT_TRUE(f$session_start());
	ASSERT_FALSE(f$session_start());
	ASSERT_TRUE(f$session_abort());
	ASSERT_TRUE(f$session_start());
	ASSERT_FALSE(f$session_start());
	f$exec(string("rm -rf ").append(session_dir_path));
}

TEST(sessions_test, test_session_start_with_params) {
	array<string> predefined_consts = array<string>();

	predefined_consts.emplace_value(string("save_path"), example_dir_path);
	ASSERT_TRUE(f$session_start(predefined_consts));
	ASSERT_TRUE(f$session_id().has_value() && !f$session_id().val().empty());
	ASSERT_TRUE(f$file_exists(string(example_dir_path).append(f$session_id().val())));
	ASSERT_TRUE(f$session_abort());

	f$exec(string("rm -rf ").append(example_dir_path));
}

TEST(sessions_test, test_session_status) {
	ASSERT_EQ(f$session_status(), SESSION_NONE);
	ASSERT_TRUE(f$session_start());
	ASSERT_EQ(f$session_status(), SESSION_ACTIVE);
	ASSERT_TRUE(f$session_abort());
	ASSERT_EQ(f$session_status(), SESSION_NONE);

	f$exec(string("rm -rf ").append(session_dir_path));
}

TEST(sessions_test, test_session_save_path) {
	f$session_start();
	ASSERT_TRUE(f$session_save_path().has_value() && !f$session_save_path().val().empty());
	ASSERT_EQ(f$session_save_path().val(), session_dir_path);
	f$session_abort();
	
	array<string> predefined_consts;
	predefined_consts.emplace_value(string("save_path"), example_dir_path);
	ASSERT_TRUE(f$session_start(predefined_consts));
	ASSERT_TRUE(f$session_save_path().has_value() && !f$session_save_path().val().empty());
	ASSERT_EQ(f$session_save_path().val(), example_dir_path);
	f$session_abort();

	f$exec(string("rm -rf ").append(session_dir_path));
	f$exec(string("rm -rf ").append(example_dir_path));
}

TEST(sessions_test, test_session_commit) {
	f$session_start();
	const string id = f$session_id().val();
	const string file_path = string(session_dir_path).append(id);

	PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION.as_array().emplace_value(string("message"), string("hello"));
	ASSERT_TRUE(f$session_commit());
	
	struct stat buf;
	int fd = open_safe(file_path.c_str(), O_RDWR, 0666);
	fstat(fd, &buf);
	char result[buf.st_size];
	ssize_t n = read_safe(fd, result, buf.st_size, file_path);
	string read_data(result, n);
	close_safe(fd);
	
	array<string> session_array;
	session_array.emplace_value(string("message"), string("hello"));
	string write_data = f$serialize(session_array);

	ASSERT_EQ(read_data, write_data);

	f$session_id(id);
	f$session_start();
	ASSERT_TRUE(f$session_id().has_value() && !f$session_id().val().empty());
	ASSERT_EQ(f$session_id().val(), id);
	PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION.as_array().emplace_value(string("message"), string("hello"));
	f$session_abort();
	ASSERT_FALSE(f$session_commit());

	fd = open_safe(file_path.c_str(), O_RDWR, 0666);
	fstat(fd, &buf);
	n = read_safe(fd, result, buf.st_size, file_path);
	read_data = string(result, n);
	ASSERT_EQ(read_data, write_data);

	f$exec(string("rm -rf ").append(session_dir_path));
}

TEST(sessions_test, test_session_decode) {
	f$session_start();
	array<mixed> data;
	data.emplace_value(string("first"), string("hello world"));
	data.emplace_value(string("second"), 60);
	data.emplace_value(3, string("smth"));

	string to_write = f$serialize(data);
	ASSERT_TRUE(f$session_decode(to_write));

	string result = f$serialize(PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION.as_array());
	ASSERT_EQ(to_write, result);

	to_write = f$serialize(array<mixed>());
	ASSERT_TRUE(f$session_decode(to_write));
	result = f$serialize(PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION.as_array());
	ASSERT_EQ(result, to_write);
	ASSERT_TRUE(f$session_abort());
	
	f$exec(string("rm -rf ").append(session_dir_path));
}

TEST(sessions_test, test_session_encode) {
	f$session_start();
	array<mixed> data;
	data.emplace_value(string("first"), string("hello world"));
	data.emplace_value(string("second"), 60);
	data.emplace_value(3, string("smth"));

	string to_read = f$serialize(data);
	PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION = data;
	ASSERT_TRUE(f$session_encode().has_value() && !f$session_encode().val().empty());
	ASSERT_EQ(f$session_encode().val(), to_read);
	ASSERT_TRUE(f$session_abort());

	f$session_start();
	ASSERT_TRUE(f$session_encode().has_value() && !f$session_encode().val().empty());
	ASSERT_EQ(f$session_encode().val(), f$serialize(array<mixed>()));
	ASSERT_TRUE(f$session_abort());
	
	f$exec(string("rm -rf ").append(session_dir_path));
}

TEST(sessions_test, test_session_reset) {
	f$session_start();
	ASSERT_TRUE(f$session_id().has_value() && !f$session_id().val().empty());
	string id = f$session_id().val();
	array<mixed> data;
	data.emplace_value(string("first"), string("hello world"));
	data.emplace_value(string("second"), 60);
	data.emplace_value(3, string("smth"));
	PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION = data;
	ASSERT_TRUE(f$session_commit());

	f$session_id(id);
	f$session_start();
	ASSERT_TRUE(f$session_id().has_value() && !f$session_id().val().empty());
	ASSERT_EQ(f$session_id().val(), id);
	array<mixed> new_data;
	new_data.emplace_value(string("first"), string("buy"));
	new_data.emplace_value(string("second"), 1000);
	PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION = new_data;
	ASSERT_TRUE(f$session_reset());
	new_data = PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION.as_array();
	ASSERT_EQ(f$serialize(data), f$serialize(new_data));
	ASSERT_TRUE(f$session_abort());

	f$exec(string("rm -rf ").append(session_dir_path));
}

TEST(sessions_test, test_session_unset) {
	f$session_start();
	array<mixed> data;
	data.emplace_value(string("first"), string("hello world"));
	PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION = data;
	ASSERT_TRUE(f$session_unset());
	data = PhpScriptMutableGlobals::current().get_superglobals().v$_SESSION.as_array();
	ASSERT_EQ(f$serialize(data), f$serialize(array<mixed>()));
	ASSERT_TRUE(f$session_abort());

	f$exec(string("rm -rf ").append(session_dir_path));
}
