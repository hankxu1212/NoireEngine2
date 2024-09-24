#include <iostream>
#include <chrono>
#include <thread>

#include "Application.hpp"

static void Parse(ApplicationCommandLineArgs args, ApplicationSpecification& spec)
{
    for (int argi = 1; argi < args.Count; ++argi)
    {
        if (strcmp(args[argi], "--drawing-size") == 0) 
        {
             if (argi + 2 >= args.Count) throw std::runtime_error("--drawing-size requires two parameters (width and height).");
             
             auto conv = [&](std::string const& what) {
                 argi++;
                 std::string val = args[argi];
                 for (size_t i = 0; i < val.size(); ++i) {
                     if (val[i] < '0' || val[i] > '9') {
                         throw std::runtime_error("--drawing-size " + what + " should match [0-9]+, got '" + val + "'.");
                     }
                 }
                 return std::stoul(val);
             };

             spec.width = conv("width");
             spec.height = conv("height");
        }
        else if (strcmp(args[argi], "--culling") == 0) 
        {
            if (argi + 1 >= args.Count) throw std::runtime_error("--culling requires one parameter (none|frustum)");
            argi++;
            if (strcmp(args[argi], "none") == 0)
                spec.Culling = ApplicationSpecification::Culling::None;
            else if (strcmp(args[argi], "frustum") == 0)
                spec.Culling = ApplicationSpecification::Culling::Frustum;
            else
                throw std::runtime_error("Not a valid culling option: " + std::string(args[argi]));
        }
        else if (strcmp(args[argi], "--physical-device") == 0) 
        {
            if (argi + 1 >= args.Count) throw std::runtime_error("--physical-device requires one parameter: physical device name");
            argi++;
            spec.PhysicalDeviceName = std::string(args[argi]);
        }
        else if (strcmp(args[argi], "--camera") == 0)
        {
            if (argi + 1 >= args.Count) throw std::runtime_error("--camera requires one parameter: camera name");
            argi++;
            spec.CameraName = std::string(args[argi]);
        }
         else {
             throw std::runtime_error("Unrecognized argument '" + std::string(args[argi]) + "'.");
        }
    }
}

static Application* CreateApplication(ApplicationCommandLineArgs args)
{
    ApplicationSpecification spec{
        .Name = "Noire Editor",
        .width = 1980,
        .height = 1020,
        .CommandLineArgs = args,
        .Culling = ApplicationSpecification::Culling::Frustum
    };

    Parse(args, spec);

    return new Application(spec);
}

int main(int argc, char** argv)
{
    try 
    {
        auto app = CreateApplication({ argc, argv });
        app->Run();
        delete app;

        std::cout << "Application successfully exited.\n\n";
        
        system("pause");
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}