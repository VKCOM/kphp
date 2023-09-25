<?php

namespace Engines014;

trait LikesEngineTrait014 {
  public static function print(): void {
    echo "like";
  }

  public static function printTweetsLikes(): void {
    \ExternalEngines014\Tweets014::printTweetsLikesCountByUser();
  }
}
