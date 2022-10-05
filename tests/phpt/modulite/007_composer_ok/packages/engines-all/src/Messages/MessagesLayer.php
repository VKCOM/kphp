<?php

namespace Engines\Messages;

class MessagesLayer {
    static public function pingRepChannels() {
        $rep_channels = Channels\ChannelsRep::createChannelsRep();
        $rep_channels->makeConnection()->logWith('MessagesLayer');

        Core\ClusterConnection::checkValid();

        // allow using built-in constants, classes, etc.
        if (0) echo PHP_INT_MAX;
        if (0) \JsonEncoder::getLastError();
        if (0) echo \JsonEncoder::rename_policy;
        if (0) new \ArrayIterator([]);
        if (0) instance_cache_delete('');
        $f = function(\Exception $ex) {};
    }
}
