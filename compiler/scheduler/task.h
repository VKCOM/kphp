// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

class Task {
public:
  virtual void execute() = 0;

  virtual ~Task() = default;
};
