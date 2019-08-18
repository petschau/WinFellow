#ifndef REMOTEDEBUGGER_H
#define REMOTEDEBUGGER_H

class HttpServerProvider;
class RemoteDebuggerAPI;

class RemoteDebugger
{
private:
  HttpServerProvider *_serverProvider;
  RemoteDebuggerAPI* _api;

public:
  bool StartWebServer();

  bool Start();
  void Stop();

  RemoteDebugger();
  ~RemoteDebugger();
};

#endif