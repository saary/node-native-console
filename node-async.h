#pragma once

#include <v8.h>
#include <TlHelp32.h>

#include <functional>
#include <memory>
#include <vector>

namespace NodeUtils
{
  using namespace v8;

  class Async
  {
  public:
    template<typename TInput, typename TResult> 
    struct Baton {
      int error_code;
      std::string error_message;

      // Custom data
      std::shared_ptr<TInput> data;
      std::shared_ptr<TResult> result;
      std::vector<Persistent<Value>> callback_args;

      void setCallbackArgs(Handle<Value>* argv, int argc)
      {
        for (int i=0; i<argc; i++)
        {
          callback_args.push_back(Persistent<Value>::New(argv[i]));
        }
      }

    private:
      uv_work_t request;
      std::function<void (Baton*)> doWork;
      std::function<void (Baton*)> afterWork;
      Persistent<Object> callbackData;

      friend Async;
    // consts
    private:
    };

    template<typename TInput, typename TResult> 
    static void __cdecl Run(
      std::shared_ptr<TInput> input, 
      std::function<void (Baton<TInput, TResult>*)> doWork, 
      std::function<void (Baton<TInput, TResult>*)> afterWork, 
      Handle<Function> callback,
      Handle<Value> receiver = Handle<Value>())
    {
      HandleScope scope;

      Handle<Object> callbackData;
      
      if (!callback.IsEmpty() && !callback->Equals(Undefined()))
      {
        callbackData = Object::New();
        
        if (!receiver.IsEmpty())
        {
          callbackData->SetPrototype(receiver);
        }

        callbackData->Set(String::NewSymbol("callback"), callback);
      
        // get the current domain:
        Handle<Value> currentDomain = Undefined();

        Handle<Object> process = Context::GetCurrent()->Global()->Get(String::NewSymbol("process")).As<Object>();
        if (!process->Equals(Undefined()))
        {
          currentDomain = process->Get(String::NewSymbol("domain")) ;
        }

        callbackData->Set(String::NewSymbol("domain"), currentDomain);
      }

      Baton<TInput, TResult>* baton = new Baton<TInput, TResult>();
      baton->request.data = baton;
      baton->callbackData = Persistent<Object>::New(callbackData);
      baton->error_code = 0;
      baton->data = input;
      baton->doWork = doWork;
      baton->afterWork = afterWork;

      uv_queue_work(uv_default_loop(), &baton->request, AsyncWork<TInput, TResult>, AsyncAfter<TInput, TResult>);
    }

    static void __cdecl RunOnMain(std::function<void ()> func)
    {
      static unsigned int uvMainThreadId = GetMainThreadId();

      if (uvMainThreadId == uv_thread_self()) 
      {
        func();
      }
      else
      {
        std::function<void ()> *funcPtr = new std::function<void ()>(func);
        uv_async_t *async = new uv_async_t;
        uv_async_init(uv_default_loop(), async, AsyncCb);
        async->data = funcPtr;
        uv_async_send(async);
      }
    }

    // defers execution of the provided function by creating an idler
    // that means, the function will be invoked once the event loop has delivered
    // all pending events.
    static void __cdecl NextTick(std::function<void ()> func)
    {
      uv_idle_t *idler = new uv_idle_t;
      std::function<void ()> *funcCopy = new std::function<void ()>(func);
      idler->data = (void *)funcCopy;
      uv_idle_init(uv_default_loop(), idler);
      uv_idle_start(idler, onNextTick);
    }

  private:
    template<typename TInput, typename TResult> 
    static void __cdecl AsyncWork(uv_work_t* req) 
    {
      // No HandleScope!

      Baton<TInput, TResult>* baton = static_cast<Baton<TInput, TResult>*>(req->data);

      // Do work in threadpool here.
      // Set baton->error_code/message on failures.
      // Set baton->result with a final result object
      baton->doWork(baton);
    }


    template<typename TInput, typename TResult> 
    static void __cdecl AsyncAfter(uv_work_t* req, int status) 
    {
      HandleScope scope;
      Baton<TInput, TResult>* baton = static_cast<Baton<TInput, TResult>*>(req->data);

      // typical AfterWorkFunc implementation
      //if (baton->error) 
      //{
      //  Handle<Value> err = Exception::Error(...);
      //  Handle<Value> argv[] = { err };
      //  baton->setCallbackArgs(argv, _countof(argv));
      //}
      //else
      //{
      //  Handle<Value> argv[] = { Undefined(), ... };
      //  baton->setCallbackArgs(argv, _countof(argv));
      //} 

      baton->afterWork(baton);
      
      if (!baton->callbackData.IsEmpty() || !baton->callbackData->Equals(Undefined()))
      {
        // call the callback, using domains and all
        int argc = static_cast<int>(baton->callback_args.size());
        std::unique_ptr<Handle<Value>> handlesArr(new Handle<Value>[argc]);
        for (int i=0; i < argc; i++)
        {
          handlesArr.get()[i] = baton->callback_args[i];
        }

        node::MakeCallback(baton->callbackData, String::NewSymbol("callback"), argc, handlesArr.get());
      }

      baton->callbackData.Dispose();
      delete baton;
    }
    
    // called after the async handle is closed in order to free it's memory
    static void __cdecl AyncCloseCb(uv_handle_t* handle) 
    {
      if (handle != nullptr)
      {
        uv_async_t* async = reinterpret_cast<uv_async_t*>(handle);
        delete async;
      }
    }

    // Called by run on main in case we are not running on the main thread
    static void __cdecl AsyncCb(uv_async_t *handle, int status)
    {
      auto func = static_cast<std::function<void ()>*>(handle->data);
      (*func)();
      uv_close((uv_handle_t*)handle, AyncCloseCb);
      delete func;
    }

    // Attributes goes to http://stackoverflow.com/a/1982200/1060807 (etan)
    static unsigned int __cdecl GetMainThreadId()
    {
      const std::shared_ptr<void> hThreadSnapshot(
        CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0), CloseHandle);

      if (hThreadSnapshot.get() == INVALID_HANDLE_VALUE)
      {
        return 0;
      }

      THREADENTRY32 tEntry;
      tEntry.dwSize = sizeof(THREADENTRY32);
      DWORD result = 0;
      DWORD currentPID = GetCurrentProcessId();

      for (BOOL success = Thread32First(hThreadSnapshot.get(), &tEntry);
        !result && success && GetLastError() != ERROR_NO_MORE_FILES;
        success = Thread32Next(hThreadSnapshot.get(), &tEntry))
      {
        if (tEntry.th32OwnerProcessID == currentPID) 
        {
          result = tEntry.th32ThreadID;
        }
      }
      return result;
    }

    static void __cdecl onNextTick(uv_idle_t *handle, int status)
    {
      std::function<void ()> *func = static_cast<std::function<void ()> *>(handle->data);
      (*func)();
      delete func;
      uv_idle_stop(handle);
      delete handle;
    }
  };
}

