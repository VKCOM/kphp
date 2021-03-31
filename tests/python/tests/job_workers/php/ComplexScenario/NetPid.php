<?php

namespace ComplexScenario;

class NetPid {
  /** @var int */
  public $ip = 0;

  /** @var int */
  public $port_pid = 0;

  /** @var int */
  public $utime = 0;

  function equals(?NetPid $other) : bool {
    if ($other === null) {
      return false;
    }
    return $this->ip === $other->ip && $this->port_pid === $other->port_pid && $this->utime === $other->utime;
  }
}
