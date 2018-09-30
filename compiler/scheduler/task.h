#pragma once

class Task {
public:
  virtual void execute() = 0;

  virtual ~Task() = default;
};
