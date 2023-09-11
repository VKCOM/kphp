<?php

namespace Logs011;

abstract class BaseLog011 {
  protected static function logAction(): bool {
    return true;
  }

  protected static function createLog(): void {
  }
}
