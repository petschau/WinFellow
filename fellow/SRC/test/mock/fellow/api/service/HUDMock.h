#ifndef TEST_MOCK_FELLOW_API_SERVICE_HUDMOCK_H
#define TEST_MOCK_FELLOW_API_SERVICE_HUDMOCK_H

#include "fellow/api/service/IHUD.h"

namespace test::mock::fellow::api::service
{
  class HUDMock : public ::fellow::api::service::IHUD
  {
  public:
    void SetFloppyLED(int driveIndex, bool active, bool write) override;
    void SetHarddiskLED(int deviceIndex, bool active, bool write) override;
  };
}

#endif
