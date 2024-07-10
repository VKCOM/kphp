#include <gtest/gtest.h>

#include "runtime/files.h"
#include "runtime/sessions.h"

TEST(sessions_test, test_session_id_with_invalid_id) {
	const string dir_path = string(getenv("TMPDIR")).append("sessions/");
	const string id = string("\t12345678\\\r");
	ASSERT_FALSE(f$session_id(id).has_value());
	ASSERT_TRUE(f$session_start());
	ASSERT_NE(f$session_id().val(), id);

	const string full_path = string(dir_path).append(f$session_id().val());
	ASSERT_TRUE(f$file_exists(full_path));
	ASSERT_TRUE(f$session_abort());
}

TEST(sessions_test, test_session_id_with_valid_id) {
	const string dir_path = string(getenv("TMPDIR")).append("sessions/");
	const string id = string("sess_668d4f818ca3b");
	ASSERT_FALSE(f$session_id(id).has_value());
	ASSERT_TRUE(f$session_start());
	ASSERT_EQ(f$session_id().val(), id);

	const string full_path = string(dir_path).append(id);
	ASSERT_TRUE(f$file_exists(full_path));
	ASSERT_TRUE(f$session_abort());
}

TEST(sessions_test, test_session_start) {
	ASSERT_TRUE(f$session_start());
	ASSERT_FALSE(f$session_start());
	ASSERT_TRUE(f$session_abort());
}

TEST(sessions_test, test_session_start_with_params) {
	array<string> predefined_consts = array<string>();
	const string dir_path = string(getenv("TMPDIR")).append("example/");
	predefined_consts.emplace_value(string("save_path"), dir_path);
	ASSERT_TRUE(f$session_start(predefined_consts));
	ASSERT_TRUE(f$file_exists(string(dir_path).append(f$session_id().val())));
	ASSERT_TRUE(f$session_abort());
}

TEST(sessions_test, test_session_status) {
	const int SESSION_NONE = 1;
	const int SESSION_ACTIVE = 2;

	ASSERT_EQ(f$session_status(), SESSION_NONE);
	ASSERT_TRUE(f$session_start());
	ASSERT_EQ(f$session_status(), SESSION_ACTIVE);
	ASSERT_TRUE(f$session_abort());
	ASSERT_EQ(f$session_status(), SESSION_NONE);
}