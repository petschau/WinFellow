#include "test/mock/fellow/api/service/HUDMock.h"

namespace test::mock::fellow::api
{
  HudConfiguration &HUDMock::GetHudConfiguration()
  {
    return _hudConfiguration;
  }
  void HUDMock::NotifyFloppyLEDChanged(int driveIndex, bool active, bool write)
  {
  }
  void HUDMock::NotifyHarddiskLEDChanged(int deviceIndex, bool active, bool write)
  {
  }

}
