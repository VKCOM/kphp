@ok
<?php

class SendBatchResult {
    var $a = 0;
}

trait IndexerTrait {
    static public function deleteBatch(): SendBatchResult { return new SendBatchResult; }
}

class IndexerBase {
    use IndexerTrait;
}

class IndexerNormal extends IndexerBase {
}

$r = IndexerNormal::deleteBatch();
echo $r->a, "\n";
