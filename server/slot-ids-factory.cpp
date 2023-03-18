// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstdlib>

#include "server/slot-ids-factory.h"

static constexpr slot_id_t MIN_SLOT_ID = 65536;
static constexpr slot_id_t MAX_SLOT_ID = 1000000000;

// these slot factories exists in each worker
// they are initialized by a random start number on request start
// and produce consecutive ids in create_slot()
// todo why they are different? merge them into one?
SlotIdsFactory rpc_ids_factory;
SlotIdsFactory parallel_job_ids_factory;
SlotIdsFactory external_db_requests_factory;


void SlotIdsFactory::init() {
  end_slot_id = begin_slot_id = static_cast<slot_id_t>(lrand48() % (MAX_SLOT_ID / 4) + MIN_SLOT_ID);
}

void init_slot_factories() {
  rpc_ids_factory.init();
  parallel_job_ids_factory.init();
  external_db_requests_factory.init();
}

void free_slot_factories() {
  // nothing to be done here, because init() doesn't acquire any resources
}
