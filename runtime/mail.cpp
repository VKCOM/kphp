// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/mail.h"

#include <cstdio>

#include "runtime/critical_section.h"
#include "runtime/interface.h"
#include "runtime/string_functions.h"

static bool check_header(const string &str) {
  int str_len = (int)str.size();
  for (int i = 0; i < str_len; i++) {
    if ((str[i] <= 31 && str[i] != '\t') || str[i] >= 127) {
      if (str[i] == '\r' && str[i + 1] == '\n' && (str[i + 2] == ' ' || str[i + 2] == '\t')) {
        i += 2;
        continue;
      }

      return false;
    }
  }
  return true;
}

bool f$mail(const string &to, const string &subject, const string &message, string additional_headers) {
  if (!check_header(to)) {
    php_warning("Wrong parameter to in function mail");
    return false;
  }
  if (!check_header(subject)) {
    php_warning("Wrong parameter subject in function mail");
    return false;
  }
  if (strlen(message.c_str()) != message.size()) {
    php_warning("Parameter message can't contain symbol with code 0 in function mail");
    return false;
  }
  if (strlen(additional_headers.c_str()) != additional_headers.size()) {
    php_warning("Parameter additional_headers can't contain symbol with code 0 in function mail");
    return false;
  }

  additional_headers = f$trim(additional_headers);

  string sendmail_path = f$ini_get(string("sendmail_path", 13)).val();
  if (sendmail_path.empty()) {
    php_warning("sendmail_path is empty. Can't send mails");
    return false;
  }
  dl::enter_critical_section();
  FILE *sendmail = popen(sendmail_path.c_str(), "w");
  if (sendmail == nullptr) {
    php_warning("Could not execute \"%s\"", sendmail_path.c_str());
    return false;
  }

  fprintf(sendmail, "To: %s\n", to.c_str());
  fprintf(sendmail, "Subject: %s\n", subject.c_str());
  if (!additional_headers.empty()) {
    fprintf(sendmail, "%s\n", additional_headers.c_str());
  }
  fprintf(sendmail, "\n%s\n", message.c_str());

  int result = pclose(sendmail);
  dl::leave_critical_section();
  if (result != 0) {
    if (result > 0) {
      php_warning("sendmail process terminated unsuccessfully");
    } else {
      php_assert(result == -1);
      php_warning("Error on waiting for sendmail process to terminate");
    }
    return false;
  }

  return true;
}
