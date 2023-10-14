#pragma once

class CoreFactory
{
public:
  static void CreateDrivers();
  static void DestroyDrivers();
  static void CreateServices();
  static void DestroyServices();
  static void CreateModules();
  static void DestroyModules();
};
