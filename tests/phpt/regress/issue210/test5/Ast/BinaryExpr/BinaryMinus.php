<?php

namespace Ast\BinaryExpr;

use Ast\BinaryExpr;

class BinaryMinus extends BinaryExpr {
  /**
   * @param \Ast\Expr $x
   * @param \Ast\Expr $y
   */
  public function __construct($x, $y) {
    parent::__construct($x, $y);
  }
}
