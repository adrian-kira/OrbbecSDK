#include "ob_updater.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <libobsensor/hpp/Error.hpp>

namespace pt = boost::property_tree;

// Static member definitions
bool OBUpdater::isWaitRebootComplete_ = false;
bool OBUpdater::isDeviceRemoved_ = false;
std::condition_variable OBUpdater::waitRebootCondition_;
std::mutex OBUpdater::waitRebootMutex_;
std::string OBUpdater::deviceUid_;
std::string OBUpdater::deviceSN_;
std::shared_ptr<ob::Device> OBUpdater::rebootedDevice_;

void OBUpdater::listDevices(bool verbose) {
  try {
    ob::Context ctx;
    if (!verbose) {
      ctx.setLoggerSeverity(OB_LOG_SEVERITY_OFF);
    }
    auto devList = ctx.queryDeviceList();

    pt::ptree root;
    pt::ptree devices;

    for (int i = 0; i < devList->deviceCount(); i++) {
      auto device = devList->getDevice(i);
      auto deviceInfo = device->getDeviceInfo();

      pt::ptree deviceNode;
      deviceNode.put("firmware_version", deviceInfo->firmwareVersion());
      deviceNode.put("serial_number", deviceInfo->serialNumber());
      deviceNode.put("name", deviceInfo->name());
      deviceNode.put("pid", deviceInfo->pid());
      deviceNode.put("vid", deviceInfo->vid());
      deviceNode.put("uid", deviceInfo->uid());

      devices.push_back(std::make_pair("", deviceNode));
    }

    root.add_child("devices", devices);

    std::ostringstream oss;
    pt::write_json(oss, root);
    std::cout << oss.str();

  } catch (const ob::Error &e) {
    std::cerr << "Error listing devices: " << e.getMessage() << "\n";
    throw;
  }
}

std::shared_ptr<ob::Device>
OBUpdater::findDeviceBySerial(const std::string &serialNumber, bool verbose) {
  ob::Context ctx;
  if (!verbose) {
    ctx.setLoggerSeverity(OB_LOG_SEVERITY_OFF);
  }
  auto devList = ctx.queryDeviceList();

  for (int i = 0; i < devList->deviceCount(); i++) {
    auto device = devList->getDevice(i);
    auto deviceInfo = device->getDeviceInfo();

    if (std::string(deviceInfo->serialNumber()) == serialNumber) {
      return device;
    }
  }

  return nullptr;
}

bool OBUpdater::updateFirmware(const std::string &serialNumber,
                               const std::string &firmwarePath, bool verbose) {
  // Check if firmware file exists
  if (!std::filesystem::exists(firmwarePath)) {
    std::cerr << "Error: Firmware file not found: " << firmwarePath << "\n";
    return false;
  }

  // Find device by serial number
  auto device = findDeviceBySerial(serialNumber, verbose);
  if (!device) {
    std::cerr << "Error: Device with serial number " << serialNumber
              << " not found\n";
    return false;
  }

  std::cout << "Found device: " << device->getDeviceInfo()->name()
            << " (SN: " << serialNumber << ")\n";

  // Store device info for reboot detection
  auto devInfo = device->getDeviceInfo();
  deviceUid_ = std::string(devInfo->uid());
  deviceSN_ = std::string(devInfo->serialNumber());

  // Setup device change callback for reboot detection
  ob::Context ctx;
  if (!verbose) {
    ctx.setLoggerSeverity(OB_LOG_SEVERITY_OFF);
  }
  ctx.setDeviceChangedCallback([](std::shared_ptr<ob::DeviceList> removedList,
                                  std::shared_ptr<ob::DeviceList> addedList) {
    if (isWaitRebootComplete_) {
      if (addedList && addedList->deviceCount() > 0) {
        auto device = addedList->getDevice(0);
        if (isDeviceRemoved_ &&
            deviceSN_ == std::string(device->getDeviceInfo()->serialNumber())) {
          rebootedDevice_ = device;
          isWaitRebootComplete_ = false;

          std::unique_lock<std::mutex> lk(waitRebootMutex_);
          waitRebootCondition_.notify_all();
        }
      }

      if (removedList && removedList->deviceCount() > 0) {
        if (deviceUid_ == std::string(removedList->uid(0))) {
          isDeviceRemoved_ = true;
        }
      }
    }
  });

  // Upgrade firmware
  std::cout << "Starting firmware upgrade...\n";
  if (!upgradeFirmware(device, firmwarePath)) {
    std::cerr << "Firmware upgrade failed\n";
    return false;
  }

  // Reboot device
  std::cout << "Rebooting device...\n";
  isDeviceRemoved_ = false;
  isWaitRebootComplete_ = true;
  device->reboot();
  device = nullptr;

  // Wait for reboot to complete
  return waitForDeviceReboot();
}

bool OBUpdater::upgradeFirmware(std::shared_ptr<ob::Device> device,
                                const std::string &firmwarePath) {
  // Check file extension
  auto extension = std::filesystem::path(firmwarePath).extension();
  if (extension != ".img" && extension != ".bin") {
    std::cerr
        << "Error: Invalid firmware file extension. Expected .img or .bin\n";
    return false;
  }

  bool isUpgradeSuccess = false;
  try {
    device->deviceUpgrade(
        firmwarePath.c_str(),
        [&isUpgradeSuccess](OBUpgradeState state, const char *message,
                            uint8_t percent) {
          switch (state) {
          case STAT_START:
            std::cout << "Firmware upgrade started\n";
            break;
          case STAT_FILE_TRANSFER:
            std::cout << "File transfer progress: "
                      << static_cast<uint32_t>(percent) << "%\n";
            break;
          case STAT_IN_PROGRESS:
            std::cout << "Upgrade progress: " << static_cast<uint32_t>(percent)
                      << "%\n";
            break;
          case STAT_DONE:
            std::cout << "Firmware upgrade completed: "
                      << static_cast<uint32_t>(percent) << "%\n";
            isUpgradeSuccess = true;
            break;
          case STAT_VERIFY_IMAGE:
            std::cout << "Verifying firmware image\n";
            break;
          default:
            std::string errMsg =
                (message != nullptr ? std::string(message) : "");
            std::cerr << "Firmware upgrade failed. State: " << state
                      << ", Error: " << errMsg
                      << ", Progress: " << static_cast<uint32_t>(percent)
                      << "%\n";
            break;
          }
        },
        false);
  } catch (const ob::Error &e) {
    std::cerr << "Firmware upgrade SDK error: " << e.getMessage() << "\n";
    return false;
  } catch (const std::exception &e) {
    std::cerr << "Firmware upgrade exception: " << e.what() << "\n";
    return false;
  }

  return isUpgradeSuccess;
}

bool OBUpdater::waitForDeviceReboot() {
  std::cout << "Waiting for device reboot to complete...\n";
  std::unique_lock<std::mutex> lk(waitRebootMutex_);

  bool rebootCompleted =
      waitRebootCondition_.wait_for(lk, std::chrono::milliseconds(60000),
                                    []() { return !isWaitRebootComplete_; });

  if (rebootedDevice_) {
    std::cout << "Device reboot completed successfully\n";
    std::cout << "Device: " << rebootedDevice_->getDeviceInfo()->name()
              << " (FW: " << rebootedDevice_->getDeviceInfo()->firmwareVersion()
              << ")\n";
    return true;
  } else {
    std::cerr << "Device reboot timed out or failed\n";
    return false;
  }
}
