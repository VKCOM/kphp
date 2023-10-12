<?php

namespace Engines015;

trait LikesEngineTrait015 {
    use FriendsEngineTrait015;
    use MusicEngineTrait015;
  public static function print(): void {
    echo "like";
  }

  public static function printTweetsLikes(): void {
    \ExternalEngines015\Tweets015::printTweetsLikesCountByUser();
  }
}
