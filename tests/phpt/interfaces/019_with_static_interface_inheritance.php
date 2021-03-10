@ok
<?php

interface IOne {
  /**
   * @param int $x
   */
  public function one($x);

  /**
   * @param int $x
   * @param int $y
   * @return mixed
   */
  public static function static_one($x, $y);
}

interface ITwo extends IOne {
  /**
   * @param int $x
   * @param int $y
   * @return int[]|int
   */
  public function two($x, $y);
}

interface IThree extends IOne {
  /**
   * @param int $x
   * @param int $y
   * @return int|null
   */
  public function three($x, $y);
}

class ImplTwo implements ITwo {
  /**
   * @param int $x
   */
  public function one($x) {
    var_dump($x);
  }

  /**
   * @param int $x
   * @param int $y
   * @return int[]
   */
  public function two($x, $y) {
    var_dump($x + $y);
    return [1];
  }

  public static function static_one($x, $y) {
    var_dump("impl_two");
  }
}

class ImplThree implements IThree {
  /**
   * @param int $x
   */
  public function one($x) {
    var_dump($x);
  }

  /**
   * @param int $x
   * @param int $y
   * @return int
   */
  public function three($x, $y) {
    var_dump($x + $y);
    return 10;
  }

  /**
   * @param int $x
   * @param int $y
   */
  public static function static_one($x, $y) {
    var_dump("impl_three");
  }
}

/** @var IOne $one */
$one = new ImplTwo();
$one->one(10);

$one = new ImplThree();
$one->one(10000);
