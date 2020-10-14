@ok
<?php
/**
 */
class Test {
  /**
   * @var self
   */
  private static $instance = null;

  /**
   * @return self
   */
  public static function instance() {
    if (!self::$instance) {
      self::$instance = new self();
    }

    return self::$instance;
  }

  /**
   * @return string
   */
  public function just() {
    return "Do it\n";
  }
}

echo Test::instance()->just();


