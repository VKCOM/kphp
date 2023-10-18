<?php

namespace Messages125;

use Logs125\BaseLog125;

class MessagesLogger125 extends BaseLog125 {

  public static function create(): void {
    parent::createLog();
  }

  public static function log(): bool {
    return parent::logAction();
  }
}
