#include "IBLUtilsApplication.hpp"
#include "LambertianEnvironmentBaker.hpp"
#include "GGXSpecularEnvironmentBaker.hpp"
#include "EnvironmentBRDFBaker.hpp"
#include "utils/Logger.hpp"

IBLUtilsApplication::IBLUtilsApplication(IBLUtilsApplicationSpecification& specs_)
{
	specs = specs_;

    ApplicationSpecification appSpecs;
    appSpecs.alternativeApplication = true;
    app = std::make_unique<Application>(appSpecs);
}

void IBLUtilsApplication::Run()
{
    if (specs.isGGX)
    {
        GGXSpecularEnvironmentBaker baker(&specs);
        baker.Run();
        NE_INFO("Baking GGX importance sampled environmental map");

        EnvironmentBRDFBaker brdfBaker(&specs);
        brdfBaker.Run();
        NE_INFO("Baking Cook-Torrance specular BRDF environmental map");
    }
    else
    {
        NE_INFO("Baking Lambertian cosine weighted environmental LUT");
        LambertianEnvironmentBaker baker(&specs);
        baker.Run();
    }
}