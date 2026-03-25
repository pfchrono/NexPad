#pragma once

#include "CXBOXController.h"
#include "NexPad.h"

class RuntimeLoop
{
public:
  RuntimeLoop(NexPad &nexPad, CXBOXController &controller);

  void tick();

private:
  CXBOXController &_controller;
  NexPad &_nexPad;
  bool _controllerWasConnected = false;
};
