// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/server/relogin.h"

#include <assert.h>
#include <grp.h>
#include <memory.h>
#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>

#include "common/kprintf.h"
#include "common/options.h"

const char *username, *groupname;

OPTION_PARSER_SHORT(OPT_GENERIC, "user", 'u', required_argument, "sets user name to make setuid") {
  if (username) {
    kprintf("wrong option -u%s, username is already defined as '%s'.\n", optarg, username);
    exit(1);
  }
  username = strdup(optarg);
  return 0;
}

OPTION_PARSER_SHORT(OPT_GENERIC, "group", 'g', required_argument, "sets group name to make setgid") {
  if (groupname) {
    kprintf("wrong option -g%s, groupname is already defined as '%s'.\n", optarg, groupname);
    exit(1);
  }
  groupname = strdup(optarg);
  return 0;
}

static bool is_empty_user_name(const char *user_name) {
  return !user_name || *user_name == '\0';
}

const char *engine_username() {
  const uid_t uid = getuid();
  const uid_t euid = geteuid();
  if (!uid || !euid) {
    if (is_empty_user_name(username)) {
      vkprintf(0, "Username not specified or empty");
      exit(1);
    }
    return username;
  }

  static uid_t cached_uid = -1;
  static char *cached_username = nullptr;
  if (cached_uid != uid) {
    const struct passwd *passwd = getpwuid(uid);
    if (!passwd) {
      vkprintf(0, "Cannot getpwuid() for %d\n", uid);
      assert("Cannot get NSS passwd entry" && passwd);
    }
    if (cached_username) {
      free(cached_username);
    }
    cached_username = strdup(passwd->pw_name);
    cached_uid = uid;
  }

  return cached_username;
}

const char *engine_groupname() {
  if (getgid() == 0 || getegid() == 0) {
    if (groupname) {
      struct group *group = getgrnam(groupname);
      if (!group) {
        vkprintf(0, "Cannot getgrnam() for %s\n", groupname);
        assert("Cannot get NSS group entry" && group);
      }

      return group->gr_name;
    }

    if (is_empty_user_name(username)) {
      vkprintf(0, "Username not specified or empty");
      exit(1);
    }

    const struct passwd *passwd = getpwnam(username);
    if (!passwd) {
      vkprintf(0, "Cannot getpwnam() for %s\n", username);
      assert("Cannot get NSS passwd entry" && passwd);
    }

    const struct group *group = getgrgid(passwd->pw_gid);
    assert(group);

    return group->gr_name;
  }

  const struct group *group = getgrgid(getgid());
  if (groupname) {
    assert(!strcmp(groupname, group->gr_name) && "Cannot change group, you are not root\n");
  }

  return group->gr_name;
}

static int change_user_group(const char *user_name, const char *group_name) {
  struct passwd *pw;
  /* lose root privileges if we have them */
  if (getuid() == 0 || geteuid() == 0) {
    if (is_empty_user_name(user_name)) {
      kprintf("You are trying to run the script as root, if you are sure of this, specify the user explicitly with the command --user\n");
      exit(1);
    }

    if (!strcmp(user_name, "root")) {
      vkprintf(0, "You are running as root");
      return 0;
    }

    if ((pw = getpwnam(user_name)) == 0) {
      kprintf("change_user_group: can't find the user %s to switch to\n", user_name);
      return -1;
    }
    gid_t gid = pw->pw_gid;
    if (setgroups(1, &gid) < 0) {
      kprintf("change_user_group: failed to clear supplementary groups list: %m\n");
      return -1;
    }

    if (group_name) {
      struct group *g = getgrnam(group_name);
      if (g == nullptr) {
        kprintf("change_user_group: can't find the group %s to switch to\n", group_name);
        return -1;
      }
      gid = g->gr_gid;
    }

    if (setgid(gid) < 0) {
      return -1;
    }

    if (setuid(pw->pw_uid) < 0) {
      kprintf("change_user_group: failed to assume identity of user %s\n", user_name);
      return -1;
    }
  } else {
    if (group_name) {
      struct group *g = getgrgid(getgid());
      if (!g || strcmp(group_name, g->gr_name)) {
        kprintf("can't change user to %s:%s, you are not root\n", user_name, group_name);
        return -1;
      }
    }
    if (username) {
      pw = getpwuid(getuid());
      if (!pw || strcmp(user_name, pw->pw_name)) {
        kprintf("can't change user to %s:%s, you are not root\n", user_name, group_name);
        return -1;
      }
    }
    return 0;
  }
  return 0;
}

void do_relogin() {
  if (change_user_group(username, groupname) < 0) {
    kprintf("fatal: cannot change user:group to %s:%s\n", engine_username(), engine_groupname());
    exit(1);
  }
}
