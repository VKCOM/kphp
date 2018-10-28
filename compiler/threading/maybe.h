#pragma once

template<class ValueType>
class Maybe {
public:
  bool has_value = false;
  char data[sizeof(ValueType)];

  Maybe() = default;

  Maybe(const ValueType &value) :
    has_value(true) {
    new(data) ValueType(value);
  }

  operator const ValueType &() const {
    assert(has_value);
    return *(ValueType *)data;
  }

  bool empty() {
    return !has_value;
  }
};
