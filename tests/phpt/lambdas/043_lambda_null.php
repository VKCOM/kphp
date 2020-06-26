<?php

/** @param callable():void $callback */
function callMe($callback) {
  /** @param callable():void $x */
  $x = $callback;
  $x = function() { echo "asdf"; };
  $x();
}
 
callMe(function() { echo "Hello"; });
callMe(null);
