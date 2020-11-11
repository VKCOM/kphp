---
sort: 2
---

# Microservices

By default, KPHP serves HTTP requests from users.  
But it can be launched as an RPC server — to serve requests from other KPHP instances.


## Available requests are described with TL scheme

Read more about [TL schema and RPC calls](../../kphp-client/tl-schema-and-rpc).

All functions to be invoked when KPHP acts like an RPC server must be annotated with `@kphp` in TL.


## Let's write a simple microservice

It can respond to *'kphp.squareNumber(n)'* request, responding *n^2*.

**First**, describe this in TL:
```
@kphp kphp.squareNumber n:int = Int;
```

**Second**, use [tl2php](../../kphp-client/tl-schema-and-rpc/tl2php.md) to generate representing PHP classes.

**Third**, write *square-microservice-backend.php* to respond to this request:
```php
$request = rpc_server_fetch_request();
if ($request instanceof \TL\kphp\Functions\kphp_squareNumber) {
  $input = $request->n;
  $output = $input * $input;
  $response = \TL\kphp\Functions\kphp_squareNumber::createRpcServerResponse($output);
  rpc_server_store_response($response);
} else {
  fwrite(STDERR, "Unsupported query\n");
}
```

**Fourth**, compile it to *kphp-squarer* binary and launch it with **--rpc-port option**:
```
./kphp-squarer --rpc-port 12321
```

Next, use another KPHP as a client to invoke the *kphp_squareNumber* RPC function.


## How to organize your project structure

If you wish, you can write code in **different repos** — each repo for each microservice. 
In this case, you'll compile them separately and deploy different binaries where you need them.

Another option is to use **monorepo** and to handle request type at the beginning of your entrypoint:
```php
if ($_SERVER['RPC_REQUEST_ID']) {
  // then it is a query for RPC server
  $rpc_response = rpc_server_fetch_request(); 
  // you handle this typed rpc query, and finally
  rpc_server_store_response($response);
  exit;
}
```
In this case, you deploy a single binary everywhere. It can be launched either as an HTTP or as an RPC server.

```danger
While serving HTTP, the master process delegates requests to worker processes.  
While serving RPC, the master process handles responses itself unless launched interconnected with a special RPC proxy (not open-sourced yet).
```
