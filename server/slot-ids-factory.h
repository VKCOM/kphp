// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <random>

using slot_id_t = int;

/**
 * Generates IDs for custom user requests in a special way, that these IDs are bound to the current script execution.
 */
class SlotIdsFactory {
public:
  /**
   * Initializes start of the IDs range with a random number.
   * Called only once in each worker on start.
   */
  void init();

  /**
   * Reinit start of the IDs range, if the last init was enough time ago.
   * Called on every request start.
   */
  void renew();

  /**
   * Shifts start of the IDs range to the end. Indicates that script is finished,
   * and any requests sent before are not from this script execution anymore.
   * Called on every request end.
   */
  void clear() { begin_slot_id = end_slot_id; }

  slot_id_t create_slot();

  /**
   * Checks if the request ID from the current script execution.
   * It works even for requests that were sent many script executions ago, because of smart renew().
   */
  bool is_from_current_script_execution(slot_id_t slot_id) const { return begin_slot_id <= slot_id && slot_id < end_slot_id; }

private:
  std::random_device rd;
  slot_id_t begin_slot_id = 0;
  slot_id_t end_slot_id = 0;
};

extern SlotIdsFactory rpc_ids_factory;
extern SlotIdsFactory parallel_job_ids_factory;
extern SlotIdsFactory external_db_requests_factory;
extern SlotIdsFactory curl_requests_factory;

void init_slot_factories();
void free_slot_factories();
void worker_global_init_slot_factories();
