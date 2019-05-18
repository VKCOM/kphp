<?php

namespace Classes;

class NamespaceTester {
  public static function f() {
    echo "__FILE__: |", __FILE__, "|\n";
    echo "__DIR__: |", __DIR__, "|\n";
    echo "__FUNCTION__: |", __FUNCTION__, "|\n";
    echo "__CLASS__: |", __CLASS__, "|\n";
    echo "__METHOD__: |", __METHOD__, "|\n";
    echo "__LINE__: |", __LINE__, "|\n";
    echo "__NAMESPACE__: |", __NAMESPACE__, "|\n";
  }

   public function g() {
    echo "__FILE__: |", __FILE__, "|\n";
    echo "__DIR__: |", __DIR__, "|\n";
    echo "__FUNCTION__: |", __FUNCTION__, "|\n";
    echo "__CLASS__: |", __CLASS__, "|\n";
    echo "__METHOD__: |", __METHOD__, "|\n";
    echo "__LINE__: |", __LINE__, "|\n";
    echo "__NAMESPACE__: |", __NAMESPACE__, "|\n";
  }
}
