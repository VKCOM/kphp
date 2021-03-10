@ok
<?php

interface ConfigReaderInterface {
  /** @return bool */
  public static function getBool();
}

class Config implements ConfigReaderInterface {
  public static function getBool() {
    return false;
  }
}
