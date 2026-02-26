/*
@brief Program for listing and updating orbbec cameras
Usage
    orbbec_fw_updater list: List all available cameras in json format. Example
output: { "devices": [ { "firmware_version": "1.6.00", "serial_number":
"CP72841000BV"} ] } orbbec_fw_updater update <serial_number> <firmware_file>:
Update the firmware of the specified camera.
*/

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <libobsensor/ObSensor.hpp>
#include <libobsensor/hpp/Error.hpp>

#include "ob_updater.hpp"

namespace po = boost::program_options;

// Function declaration
void printUsage(const po::options_description &desc);

int main(int argc, char *argv[]) {
  try {
    po::options_description desc("Options");
    desc.add_options()("help,h", "Show this help message")(
        "verbose,v", "Enable verbose output (show SDK debug messages)");

    po::positional_options_description pos_desc;
    pos_desc.add("command", 1);
    pos_desc.add("args", -1);

    po::options_description hidden("Hidden options");
    hidden.add_options()("command", po::value<std::string>(), "command")(
        "args", po::value<std::vector<std::string>>(), "arguments");

    po::options_description all_options;
    all_options.add(desc).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
                  .options(all_options)
                  .positional(pos_desc)
                  .run(),
              vm);
    po::notify(vm);

    if (vm.count("help") || argc == 1) {
      printUsage(desc);
      return 0;
    }

    // Configure SDK logging based on verbose flag
    bool verbose = vm.count("verbose") > 0;

    // Handle positional command
    if (vm.count("command")) {
      const std::string command = vm["command"].as<std::string>();

      if (command == "list") {
        OBUpdater::listDevices(verbose);
        return 0;
      } else if (command == "update") {
        if (!vm.count("args")) {
          std::cerr << "Error: update requires exactly 2 arguments: "
                       "<serial_number> <firmware_file>\n";
          printUsage(desc);
          return 1;
        }
        const auto &args = vm["args"].as<std::vector<std::string>>();
        if (args.size() != 2) {
          std::cerr << "Error: update requires exactly 2 arguments: "
                       "<serial_number> <firmware_file>\n";
          printUsage(desc);
          return 1;
        }

        const std::string &serialNumber = args[0];
        const std::string &firmwarePath = args[1];

        if (OBUpdater::updateFirmware(serialNumber, firmwarePath, verbose)) {
          std::cout << "Firmware update completed successfully\n";
          return 0;
        } else {
          std::cerr << "Firmware update failed\n";
          return 1;
        }
      } else {
        std::cerr << "Error: Unknown command '" << command << "'\n";
        printUsage(desc);
        return 1;
      }
    }

    std::cerr << "Error: No valid command specified\n";
    printUsage(desc);
    return 1;

  } catch (const po::error &e) {
    std::cerr << "Command line parsing error: " << e.what() << "\n";
    return 1;
  } catch (const ob::Error &e) {
    std::cerr << "Orbbec SDK error - function: " << e.getName()
              << ", args: " << e.getArgs() << ", message: " << e.getMessage()
              << ", type: " << e.getExceptionType() << "\n";
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}

void printUsage(const po::options_description &desc) {
  std::cout << "Orbbec Firmware Updater\n\n";
  std::cout << "Usage:\n";
  std::cout << "  orbbec_fw_updater list                                    "
               "List all cameras\n";
  std::cout << "  orbbec_fw_updater update <serial_number> <firmware_file> "
               "Update firmware\n\n";
  std::cout << desc << "\n";
}