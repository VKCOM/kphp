<?php

namespace Messages011;

use Logs011\BaseLog011;

class MessagesLogger011 extends BaseLog011 {

  public static function create(): void {
    parent::createLog();
  }

  public static function log(): bool {
    return parent::logAction();
  }
}
