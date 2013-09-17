#pragma once

#include <v8.h>
#include <uv.h>
#include <functional>
#include "node-async.h"

namespace NodeUtils
{
  using namespace v8;

  class Console 
  {
  public:
    Console()
    {
      ConnectToJSConsole();
    }

    ~Console()
    {
      Dispose();
    }

    // may be called from main thread or worker threads / RT event threads
    void Log(const wchar_t *wstr) { Async::RunOnMain([&, wstr] { Log(_log, wstr); }); }
    void Info(const wchar_t *wstr) { Async::RunOnMain([&, wstr] { Log(_info, wstr); }); }
    void Warn(const wchar_t *wstr) { Async::RunOnMain([&, wstr] { Log(_warn, wstr); }); }
    void Error(const wchar_t *wstr) { Async::RunOnMain([&, wstr] { Log(_error, wstr); }); }

    void ConnectToJSConsole()
    {
      Dispose();

      HandleScope scope;

      _console = Persistent<Object>::New(v8::Context::GetCurrent()->Global()->Get(String::NewSymbol("console")).As<Object>());
      if (!_console.IsEmpty())
      {
        _log = Persistent<Function>::New(_console->Get(String::NewSymbol("log")).As<Function>());
        _info = Persistent<Function>::New(_console->Get(String::NewSymbol("info")).As<Function>());
        _warn = Persistent<Function>::New(_console->Get(String::NewSymbol("warn")).As<Function>());
        _error = Persistent<Function>::New(_console->Get(String::NewSymbol("error")).As<Function>());
      }
    }

  private:
    void Dispose()
    {
      _console.Dispose();
      _log.Dispose();
      _info.Dispose();
      _warn.Dispose();
      _error.Dispose();
    }

    void Log(Handle<Function>& logFunction, const wchar_t *wstr)
    {
      HandleScope scope;
      Local<String> str = String::New(wstr);
      Log(logFunction, str);
    }

    void Log(Handle<Function>& logFunction, Handle<String>& str)
    {
      if (!logFunction.IsEmpty())
      {
        HandleScope scope;
        const unsigned argc = 1;
        Local<Value> argv[argc] = { Local<Value>::New(str) };

        logFunction->Call(_console, argc, argv);
      }
    }

  private:
    Persistent<Object> _console;
    Persistent<Function> _log;
    Persistent<Function> _info;
    Persistent<Function> _warn;
    Persistent<Function> _error;
  };
}


