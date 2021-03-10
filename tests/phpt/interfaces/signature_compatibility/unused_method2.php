@ok
KPHP_REQUIRE_FUNCTIONS_TYPING=1
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
