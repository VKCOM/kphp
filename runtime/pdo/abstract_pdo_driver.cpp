// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/pdo/abstract_pdo_driver.h"

#include "server/external-net-drivers/net-drivers-adaptor.h"

namespace pdo {

AbstractPdoDriver::~AbstractPdoDriver() noexcept {
  vk::singleton<NetDriversAdaptor>::get().remove_connector(connector_id);
}

} // namespace pdo
