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
   * @return mixed
   */
  public function two($x, $y);
}

interface IThree extends IOne {
  /**
   * @param int $x
   * @param int $y
   * @return mixed
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
   * @return void|mixed
   */
  public function two($x, $y) {
    var_dump($x + $y);
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
   * @return void|mixed
   */
  public function three($x, $y) {
    var_dump($x + $y);
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
