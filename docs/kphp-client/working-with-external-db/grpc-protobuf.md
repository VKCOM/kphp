---
sort: 4
---

# gRPC, protobuf

gRPC can be done with "curl over HTTP/2" — in a request-response manner, but without streaming.

Protobuf encoding is not a part of KPHP but is easily made with PHP.  
Standard PHP-proto-generators are not suitable, because they generate too abstract code — with accessing fields by dynamic names and other non-compilable approaches.

**Here is a KPHP-compatible library for Protobuf**: [/kphp-snippets/protobuf]({{site.url_github_kphp_snippets}}/tree/master/Protobuf).  
It contains all necessary documentation and can be copy-pasted without modification.  
For now, you manually write PHP classes representing the .proto scheme, but it's very fast and easy.  

**Here is a KPHP-compatible library for gRPC**: [/kphp-snippets/grpc]({{site.url_github_kphp_snippets}}/tree/master/Grpc).  
It sends Protobuf-encoded data over HTTP/2 and can be copy-pasted with slight modifications.  
It is based on curl_multi invocations and can be used asynchronously.

```note
Protobuf can be made much effective if embedded to KPHP natively, but for now, this library is enough.  
С++ extension `grpc-core` can't be integrated, as it creates several threads in the background, whereas KPHP workers are fundamentally single-threaded. 
```
