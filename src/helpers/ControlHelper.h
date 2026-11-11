#include "../../examples/simple_repeater/MyMesh.h"

class ControlHelperClass
{
public:
  void Init(MyMesh &mesh);
  void SendStatus();
  bool sendMessage(const char *messageText);
};
