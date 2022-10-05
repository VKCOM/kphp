<?php

class Either008 {
  /** @var mixed */
  public $data = null;
  /** @var mixed[] */
  public $errors = [];

  public static function isError(object $either): bool {
    return count($either->errors) > 0;
  }

  /**
   * @kphp-generic TEither, TData
   * @param TEither $either
   * @param TData $data
   * @return TEither
   */
  public static function data($either, $data) {
    $either->data   = $data;
    $either->errors = [];
    return $either;
  }

  /**
   * @kphp-generic TEither, TDef
   * @param TEither $either
   * @param mixed $error
   * @param TDef $default_value
   * @return TEither
   */
  public static function error($either, $error, $default_value) {
    $either->data   = $default_value;
    $either->errors = [$error];
    return $either;
  }

  /**
   * @kphp-generic T
   * @param T $either
   * @return T::data
   */
  public static function getData($either) {
    return $either->data;
  }

}
