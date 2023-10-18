<?php

namespace Printer012;

use BasePrinter012\BasePrinter012;

class Printer012 extends BasePrinter012 {

  public static function print(): void {
    self::basePrint();
    echo "print\n";
  }
}
