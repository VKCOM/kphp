<?php

namespace Printer013;

use BasePrinter013\BasePrinter013;

class Printer013 extends BasePrinter013 {
  public static function print(): void {
    self::printAppend();
    echo "print-printAppend";
  }
}
