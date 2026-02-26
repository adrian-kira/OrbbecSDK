#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <libobsensor/ObSensor.hpp>

class OBUpdater {
public:
  // Static methods for device management and firmware updates
  static void listDevices(bool verbose = true);
  static std::shared_ptr<ob::Device>
  findDeviceBySerial(const std::string &serialNumber, bool verbose = true);
  static bool updateFirmware(const std::string &serialNumber,
                             const std::string &firmwarePath,
                             bool verbose = true);
  static bool upgradeFirmware(std::shared_ptr<ob::Device> device,
                              const std::string &firmwarePath);
  static bool waitForDeviceReboot();

private:
  // Static variables for device reboot handling
  static bool isWaitRebootComplete_;
  static bool isDeviceRemoved_;
  static std::condition_variable waitRebootCondition_;
  static std::mutex waitRebootMutex_;
  static std::string deviceUid_;
  static std::string deviceSN_;
  static std::shared_ptr<ob::Device> rebootedDevice_;
};
