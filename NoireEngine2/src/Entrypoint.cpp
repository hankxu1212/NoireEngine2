#pragma once

#include <iostream>

#include "Application.hpp"

static Application* CreateApplication(ApplicationCommandLineArgs args)
{
    ApplicationSpecification spec{
        .Name = "Editor",
        .WorkingDirectory = "../NoireEngine/src/resources",
        .width = 1980,
        .height = 1020,
        .CommandLineArgs = args,
    };

    return new Application(spec);
}

int main(int argc, char** argv)
{
    try {
        auto app = CreateApplication({ argc, argv });
        app->Run();
        delete app;
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}