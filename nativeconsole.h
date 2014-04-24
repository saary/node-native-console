// Copyright (c) Microsoft Corporation
// All rights reserved. 
//
// Licensed under the Apache License, Version 2.0 (the ""License""); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 
//
// THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT. 
//
// See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.

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
    void Log(const wchar_t *wstr) const { Async::RunOnMain([&, wstr] { Log(_logSymbol, wstr); }); }
    void Info(const wchar_t *wstr) const { Async::RunOnMain([&, wstr] { Log(_infoSymbol, wstr); }); }
    void Warn(const wchar_t *wstr) const { Async::RunOnMain([&, wstr] { Log(_warnSymbol, wstr); }); }
    void Error(const wchar_t *wstr) const { Async::RunOnMain([&, wstr] { Log(_errorSymbol, wstr); }); }

    void ConnectToJSConsole()
    {
      Dispose();

      HandleScope scope;

      _console = Persistent<Object>::New(Object::New());
      _console->SetPrototype(v8::Context::GetCurrent()->Global()->Get(String::NewSymbol("console")).As<Object>());

      // attach domain:
      // get the current domain:
      Handle<Value> currentDomain;

      Handle<Object> process = v8::Context::GetCurrent()->Global()->Get(String::NewSymbol("process")).As<Object>();
      if (!process->Equals(Undefined()))
      {
        currentDomain = process->Get(String::NewSymbol("domain")) ;
      }

      if (!currentDomain.IsEmpty() && !currentDomain->Equals(Undefined())) 
      {
        _console->Set(String::NewSymbol("domain"), currentDomain);
      }

      if (!_console.IsEmpty())
      {
        _logSymbol = Persistent<String>::New(String::NewSymbol("log"));
        _infoSymbol = Persistent<String>::New(String::NewSymbol("info"));
        _warnSymbol = Persistent<String>::New(String::NewSymbol("warn"));
        _errorSymbol = Persistent<String>::New(String::NewSymbol("error"));
      }
    }

  private:
    void Dispose()
    {
      _console.Dispose();
      _logSymbol.Dispose();
      _infoSymbol.Dispose();
      _warnSymbol.Dispose();
      _errorSymbol.Dispose();
    }

    void Log(const Handle<String>& logFunctionSymbol, const wchar_t *wstr) const
    {
      HandleScope scope;
      Local<String> str = String::New(wstr);
      Log(logFunctionSymbol, str);
    }

    void Log(const Handle<String>& logFunctionSymbol, Handle<String>& str) const
    {
      if (!logFunctionSymbol.IsEmpty())
      {
        HandleScope scope;
        Local<Value> argv[] = { Local<Value>::New(str) };
        node::MakeCallback(_console, logFunctionSymbol, _countof(argv), argv);
      }
    }

  private:
    Persistent<Object> _console;
    Persistent<String> _logSymbol;
    Persistent<String> _infoSymbol;
    Persistent<String> _warnSymbol;
    Persistent<String> _errorSymbol;
  };
}


