#include <v8.h>

using namespace v8;
void logUvAsyncCb(uv_async_t *, int);
void infoUvAsyncCb(uv_async_t *, int);
void warnUvAsyncCb(uv_async_t *, int);
void errorUvAsyncCb(uv_async_t *, int);

namespace NodeUtils
{

  class Console 
  {
  public:
    Console()
    {
      ConnectToJSConsole();
      SetupThreadMessageReceivers();
    }

    ~Console()
    {
      Dispose();
    }

    void Log(Handle<String> str)
    {
      if (uvThreadID != GetCurrentThreadId()) return; // other threads may not pass in Handles (like, Local<>)
      Log(_log, str); 
    }
    
    void Log(const wchar_t* data)
    {
      if (uvThreadID == GetCurrentThreadId()) {
        Log(_log, String::New(data));
      } else {
        _logUvAsync.data = (void *)data;
        uv_async_send(&_logUvAsync);
      }
    }

    void Info(Handle<String> str)
    {
      if (uvThreadID != GetCurrentThreadId()) return;
      Log(_info, str); 
    }
    
    void Info(const wchar_t* str)
    {
      if (uvThreadID == GetCurrentThreadId()) {
        Log(_info, String::New(str));
      } else {
        _infoUvAsync.data = (void *)str;
        uv_async_send(&_infoUvAsync);
      }
    }

    void Warn(Handle<String> str)
    {
      if (uvThreadID != GetCurrentThreadId()) return; 
      Log(_warn, str); 
    }
    
    void Warn(const wchar_t* str)
    {
      if (uvThreadID == GetCurrentThreadId()) {
        Log(_warn, String::New(str));
      } else {
        _warnUvAsync.data = (void *)str;
        uv_async_send(&_warnUvAsync);
      }
    }

    void Error(Handle<String> str)
    {
      if (uvThreadID != GetCurrentThreadId()) return;
      Log(_error, str); 
    }
    void Error(const wchar_t* str)
    {
      if (uvThreadID == GetCurrentThreadId()) {
        Log(_error, String::New(str));
      } else {
        _errorUvAsync.data = (void *)str;
        uv_async_send(&_errorUvAsync);
      }
    }

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
    
    void SetupThreadMessageReceivers()
    {
      uv_async_init(uv_default_loop(), &_logUvAsync, logUvAsyncCb);
      uv_async_init(uv_default_loop(), &_infoUvAsync, infoUvAsyncCb);
      uv_async_init(uv_default_loop(), &_warnUvAsync, warnUvAsyncCb);
      uv_async_init(uv_default_loop(), &_errorUvAsync, errorUvAsyncCb);
      uvThreadID = GetCurrentThreadId(); // the constructor is being called from the main (event-loop) thread.
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
    DWORD uvThreadID;
    uv_async_t _logUvAsync;
    uv_async_t _infoUvAsync;
    uv_async_t _warnUvAsync;
    uv_async_t _errorUvAsync;
  };
}

//needed for functions below
static NodeUtils::Console console;

/* Helper functions running in the main thread that receive the 
 * string to be printed from a different thread */
void logUvAsyncCb(uv_async_t *handle, int status)
{
	console.Log(String::New((wchar_t *)(handle->data)));
}

void infoUvAsyncCb(uv_async_t *handle, int status)
{
	console.Info(String::New((wchar_t *)(handle->data)));
}

void warnUvAsyncCb(uv_async_t *handle, int status)
{
	console.Warn(String::New((wchar_t *)(handle->data)));
}

void errorUvAsyncCb(uv_async_t *handle, int status)
{
	console.Error(String::New((wchar_t *)(handle->data)));
}

