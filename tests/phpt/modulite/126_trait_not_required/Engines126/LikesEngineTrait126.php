<?php

namespace Engines126;

trait LikesEngineTrait126 {
  public static function print(): void {
    echo "like";
  }

  public static function printTweetsLikes(): void {
    \ExternalEngines126\Tweets126::printTweetsLikesCountByUser();
    $_ = new \ExternalEngines126\Tweets126();
  }
}
