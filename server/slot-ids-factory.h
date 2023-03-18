// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

using slot_id_t = int;

class SlotIdsFactory {
public:
  void init();
  
  slot_id_t create_slot() { return end_slot_id++; }
  bool is_valid_slot(slot_id_t slot_id) const { return begin_slot_id <= slot_id && slot_id < end_slot_id; }

private:
  slot_id_t begin_slot_id = 0;
  slot_id_t end_slot_id = 0;
};

extern SlotIdsFactory rpc_ids_factory;
extern SlotIdsFactory parallel_job_ids_factory;
extern SlotIdsFactory external_db_requests_factory;

void init_slot_factories();
void free_slot_factories();
