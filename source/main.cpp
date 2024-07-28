// STL modules
#include <filesystem>

// Custom modules
#include "common/astronomy.hpp"
#include "common/utility.hpp"
#include "common/config.hpp"
#include "common/http_server.hpp"
using namespace kc;

struct ParseResult
{
    enum class Result
    {
        None,       // Parsing error occured
        ShowHelp,   // Help message was requested
        Generate,   // Necessary files generation was requested
        Start,      // Normal Copaipy start was requested
    };

    const char* executableName;
    Result result;
    bool forceColor;
};

/// @brief Parse commmandline arguments
/// @param argc Count of arguments
/// @param argv Values of arguments
/// @return Parse result
static ParseResult ParseOptions(int argc, char** argv)
{
    ParseResult result;
    result.executableName = argv[0];
    result.result = ParseResult::Result::Start;
    result.forceColor = false;

    for (int index = 1; index < argc; ++index)
    {
        std::string option = argv[index];

        if (option == "-fc" || option == "--force-color")
        {
            result.forceColor = true;
            continue;
        }

        if (result.result != ParseResult::Result::Start)
        {
            fmt::print("Ignoring option: \"{}\"\n", option);
            continue;
        }

        if (option == "-h" || option == "--help")
        {
            result.result = ParseResult::Result::ShowHelp;
            continue;
        }

        if (option == "-g" || option == "--generate")
        {
            result.result = ParseResult::Result::Generate;
            continue;
        }

        fmt::print(
            "Unknown option: \"{}\"\n"
            "See {} --help\n",
            option,
            result.executableName
        );

        result.result = ParseResult::Result::None;
        return result;
    }

    return result;
}

/// @brief Show help message
/// @param result Commandline arguments parse result
static int ShowHelpMessage(const ParseResult& result)
{
    fmt::print(
        "Copaipy usage: {} [OPTIONS]\n"
        "Available options:\n"
        "    (No options)\tStart Copaipy normally\n"
        "    -fc, --force-color\tForce colored logs regardless of whether your tty supports them or not\n"
        "Unique options:\n"
        "    -h, --help\t\tShow this message and exit\n"
        "    -g, --generate\tGenerate necessary files and exit\n"
        "Only one of the unique options may be passed at the same time. All others will be ignored.\n",
        result.executableName
    );
    return 0;
}

/// @brief Generate necessary files
/// @return Executable exit code
static int GenerateFiles()
{
    if (std::filesystem::is_regular_file(ConfigConst::ConfigFile))
    {
        fmt::print(
            "Configuration file \"{}\" already exists.\n"
            "Delete it first to confirm that you don't care about its contents.\n",
            ConfigConst::ConfigFile
        );
        return 1;
    }

    try
    {
        Config::GenerateSampleFile();
    }
    catch (...)
    {
        fmt::print(
            "Couldn't create configuration file \"{}\".\n"
            "Please check permissions.\n",
            ConfigConst::ConfigFile
        );
        return 1;
    }

    fmt::print(
        "Configuration file \"{}\" was created.\n"
        "Please configure the file before starting Copaipy.\n",
        ConfigConst::ConfigFile
    );
    return 0;
}

/// @brief Initialize config
/// @return Initialized config
static Config::Pointer Init(const ParseResult& result)
{
    spdlog::logger logger = Utility::CreateLogger("init", result.forceColor);
    try
    {
        return std::make_shared<Config>();
    }
    catch (const Config::Error& error)
    {
        logger.error("Configuration error: {}", error.what());
        logger.info("Hint: Check configuration file \"{}\"", ConfigConst::ConfigFile);
        logger.info("Hint: You can generate necessary files by running {} --generate", result.executableName);
        return {};
    }
}

int main(int argc, char** argv)
{
    ParseResult result = ParseOptions(argc, argv);
    switch (result.result)
    {
        case ParseResult::Result::None:
            return 1;
        case ParseResult::Result::ShowHelp:
            return ShowHelpMessage(result);
        case ParseResult::Result::Generate:
            return GenerateFiles();
        default:
            break;
    }

    Config::Pointer config = Init(result);
    if (!config)
        return 1;

    /*
    fmt::print(
        "Welcome to Copaipy\n"
        "GitHub repository: https://github.com/KontraCity/Copaipy\n"
    );
    */

    spdlog::logger logger = Utility::CreateLogger("main");
    try
    {
        dt::date date = dt::day_clock::local_day();
        pt::ptime sunrise = Astronomy::CalculateSunrise(config, date);
        pt::ptime sunset = Astronomy::CalculateSunset(config, date);

        fmt::print("Date: {:02d}.{:02d}.{:04d}\n", date.day().as_number(), date.month().as_number(), static_cast<int>(date.year()));
        fmt::print("Sunrise: {:02d}:{:02d} (angle: {:.2f} degrees)\n", sunrise.time_of_day().hours(), sunrise.time_of_day().minutes(), config->sunriseAngle());
        fmt::print("Sunset: {:02d}:{:02d} (angle: {:.2f} degrees)\n", sunset.time_of_day().hours(), sunset.time_of_day().minutes(), config->sunsetAngle());

        /*
        Display::Ui::Pointer displayUi = std::make_shared<Display::Ui>(config);
        displayUi->enable();

        HttpServer server(config, displayUi);
        server.start();
        */
    }
    catch (const std::exception& error)
    {
        logger.critical("Exception: \"{}\"", error.what());
        logger.critical("Copaipy is terminating");
        return -1;
    }
}
