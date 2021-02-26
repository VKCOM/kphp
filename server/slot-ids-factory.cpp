// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstdlib>

#include "server/slot-ids-factory.h"

void SlotIdsFactory::init() {
  end_slot_id = begin_slot_id = static_cast<slot_id_t>(lrand48() % (max_slot_id / 4) + 1);
}

slot_id_t SlotIdsFactory::create_slot() {
  if (end_slot_id > max_slot_id) {
    return -1;
  }
  return end_slot_id++;
}

bool SlotIdsFactory::is_valid_slot(slot_id_t slot_id) const {
  return begin_slot_id <= slot_id && slot_id < end_slot_id;
}

void SlotIdsFactory::clear() {
  begin_slot_id = end_slot_id;
  if (begin_slot_id > max_slot_id / 2) {
    init();
  }
}
