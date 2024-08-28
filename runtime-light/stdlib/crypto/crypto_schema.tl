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

---functions---

getCryptosecurePseudorandomBytes#2491b81d
    size : int
    = Maybe string;

getPemCertInfo#a50cfd6c
    short : Bool
    bytes : string
    = Maybe (Dictionary CertInfoItem);
