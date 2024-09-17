// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"


template<class InstanceClass>
Optional<string> f$instance_serialize(const class_instance<InstanceClass> &instance) noexcept {
  php_critical_error("call to unsupported function");
}


template<class InstanceClass>
string f$instance_serialize_safe(const class_instance<InstanceClass> &instance) noexcept {
  php_critical_error("call to unsupported function");
}

template<class ResultClass>
ResultClass f$instance_deserialize(const string &buffer, const string&) noexcept {
  php_critical_error("call to unsupported function");
}

template<class ResultClass>
ResultClass f$instance_deserialize_safe(const string &buffer, const string&) noexcept {
  php_critical_error("call to unsupported function");
}
