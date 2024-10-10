#include <iostream>
#include <chrono>
#include <thread>

#include "Application.hpp"
#include "IBLUtilsApplication.h"

[[maybe_unused]] static void Parse(ApplicationCommandLineArgs args, IBLUtilsApplicationSpecification& spec)
{
    if (args.Count != 4)
        throw std::runtime_error("Incorrect number of arguments. Must take exactly 3 arguments: inFile, --method, outFile");

    spec.inFile = args[1];
    spec.outFile = args[3];

    if (strcmp(args[2], "--lambertian") == 0)
        spec.isGGX = false;
    else if (strcmp(args[2], "--ggx") == 0)
        spec.isGGX = true;
    else
        throw std::runtime_error("Parsing isGGX parameter failed. Got:" + std::string(args[2]));
}


static IBLUtilsApplication* CreateApplication(ApplicationCommandLineArgs args)
{
    IBLUtilsApplicationSpecification spec;

    spec.inFile = "../scenes/examples/ox_bridge_morning.png";
    spec.isGGX = false;
    spec.outFile = "../scenes/examples/AA.png";
    //Parse(args, spec);

    return new IBLUtilsApplication(spec);
}

int main(int argc, char** argv)
{
    try
    {
        auto app = CreateApplication({ argc, argv });
        app->Run();
        delete app;

        std::cout << "IBL Application successfully exited.\n\n";

        system("pause");
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}