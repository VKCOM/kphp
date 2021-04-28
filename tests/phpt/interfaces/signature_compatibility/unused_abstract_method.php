@ok
<?php

abstract class ConfigReader {
  /** @return bool */
  abstract public static function getBool();
}

class Config extends ConfigReader {
  public static function getBool() {
    return false;
  }
}
