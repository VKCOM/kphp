// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

using slot_id_t = int;

class SlotIdsFactory {
public:
  void init();
  slot_id_t create_slot();
  bool is_valid_slot(slot_id_t slot_id) const;
  void clear();

private:
  slot_id_t begin_slot_id = 0;
  slot_id_t end_slot_id = 0;
  slot_id_t max_slot_id = 1000000000;
};
