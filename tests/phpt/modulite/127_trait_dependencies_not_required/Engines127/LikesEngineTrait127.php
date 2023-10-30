<?php

namespace Engines127;

trait LikesEngineTrait127 {
  public static function print(): void {
    echo "like";
  }

  public static function printTweetsLikes(): void {
    \ExternalEngines127\Tweets127::printTweetsLikesCountByUser();
    $_ = new \ExternalEngines127\Tweets127();
  }
}
