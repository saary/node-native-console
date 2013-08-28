#include <node.h>
#include <v8.h>
#include "nativeconsole.h"

using namespace v8;
using namespace NodeUtils;

Console console;

Handle<Value> Log(const Arguments& args) {
  HandleScope scope;

  if (args.Length() > 0) 
  {
    console.Log(args[0]->ToString());    
  }
  
  console.Log(L"just another.");    

  return scope.Close(Undefined());
}

void init(Handle<Object> exports) {
  exports->Set(String::NewSymbol("log"),
      FunctionTemplate::New(Log)->GetFunction());
}

NODE_MODULE(logger, init)