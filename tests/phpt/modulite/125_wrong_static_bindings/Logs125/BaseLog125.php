<?php

namespace Logs125;

abstract class BaseLog125 {
  protected static function logAction(): bool {
    return true;
  }

  protected static function createLog(): void {
  }
}
