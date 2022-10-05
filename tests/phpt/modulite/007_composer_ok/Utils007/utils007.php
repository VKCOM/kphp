<?php

function utilsSummarize() {
    $b = new \VK\Strings\StringBuilder007('Hello ');
    $b->append('world');
    echo $b->get(), "\n";

    echo \VK\Strings\Utils007\Append007::concatStr('utils', 'concat'), "\n";

    pingAllEngines();
    \Engines\Messages\MessagesLayer::pingRepChannels();
    \Engines\Messages\Channels\ChannelsRep::createChannelsRep()->logWith('utilsSummarize');
}

