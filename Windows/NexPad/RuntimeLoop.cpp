#include "RuntimeLoop.h"

RuntimeLoop::RuntimeLoop(NexPad &nexPad, CXBOXController &controller)
    : _controller(controller),
      _nexPad(nexPad)
{
}

void RuntimeLoop::tick()
{
  const XINPUT_STATE state = _controller.GetState();
  if (!_controller.GetLastConnectionState())
  {
    if (_controllerWasConnected)
    {
      _nexPad.onControllerDisconnected();
    }

    _controllerWasConnected = false;
    return;
  }

  _controllerWasConnected = true;
  _nexPad.setCurrentState(state);
  _nexPad.processCurrentState();
}
