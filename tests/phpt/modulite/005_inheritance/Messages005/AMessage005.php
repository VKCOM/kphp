<?php

namespace Messages005;

abstract class AMessage005 {
  const ID = 10;
  static string $NAME = "msg_name";

  static function getName(): string {
    return 'getName()';
  }
}

