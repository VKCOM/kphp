<?php

define('START_MODE', 'start');

function pingAllEngines() {
    Engines\Messages\MessagesLayer::pingRepChannels();

    $rep_folders = Engines\Messages\Folders\FoldersRep::createFoldersRep();
    $rep_folders->log();

    if (0) echo MONOLITH_CONST_007;
}
