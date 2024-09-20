---types---

// Boolean
boolFalse#bc799737 = Bool;
boolTrue#997275b5 = Bool;

// Maybe
resultFalse#27930a7b {t:Type} = Maybe t;
resultTrue#3f9c8ef8 {t:Type} result:t = Maybe t;

// Vector
vector#1cb5c415 {t:Type} # [t] = Vector t;

// Tuple
tuple#9770768a {t:Type} {n:#} [t] = Tuple t n;

// Dictionary
dictionaryField {t:Type} key:string value:t = DictionaryField t;
dictionary#1f4c618f {t:Type} %(Vector %(DictionaryField t)) = Dictionary t;

certInfoItemLong#533ff89f l:long = CertInfoItem;
certInfoItemStr#c427feef s:string = CertInfoItem;
certInfoItemDict#1ea8a774 d: %(Dictionary string) = CertInfoItem;

digestAlgorithmDSS1#f572d6b6 = DigestAlgorithm; // DSA digest + with SHA1 hash function
digestAlgorithmSHA1#215fb97d = DigestAlgorithm;
digestAlgorithmSHA224#8bce55e9 = DigestAlgorithm;
digestAlgorithmSHA256#6c977f8c = DigestAlgorithm;
digestAlgorithmSHA384#f54c2608 = DigestAlgorithm;
digestAlgorithmSHA512#225df2b6 = DigestAlgorithm;
digestAlgorithmRMD160#1887e6b4 = DigestAlgorithm;
digestAlgorithmMD5#257ddf13 = DigestAlgorithm;
digestAlgorithmMD4#317fe3d1 = DigestAlgorithm;
digestAlgorithmMD2#5aca6998 = DigestAlgorithm;

---functions---

getCryptosecurePseudorandomBytes#2491b81d
    size : int
    = Maybe string;

getPemCertInfo#a50cfd6c
    short : Bool
    bytes : string
    = Maybe (Dictionary CertInfoItem);

digestSign#d345f658
    data         : string
    private_key  : string
    algorithm    : DigestAlgorithm
    = Maybe string;

digestVerify#5760bd0e
    data        : string
    private_key : string
    algorithm   : DigestAlgorithm
    signature   : string
    = Bool;
