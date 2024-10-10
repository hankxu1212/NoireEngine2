#include "IBLUtilsApplication.h"

#include "core/resources/Files.hpp"
#include "utils/Logger.hpp"
#include "core/Bitmap.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

IBLUtilsApplication::IBLUtilsApplication(IBLUtilsApplicationSpecification& specs_)
{
	specs = specs_;
}

IBLUtilsApplication::~IBLUtilsApplication()
{
}

const float pi_over_4 = glm::pi<float>() / 4;
const float pi_over_2 = glm::pi<float>() / 2;

static inline glm::vec2 SampleConcentricDisc(glm::vec2 in)
{
    glm::vec2 offset = in * 2.0f - 1.0f;
    if (offset == glm::vec2(0))
        return glm::vec2(0);

    float theta, r;

    if (std::abs(offset.x) > std::abs(offset.y)) {
        r = offset.x;
        theta = pi_over_4 * (offset.y / offset.x);
    }
    else {
        r = offset.y;
        theta = pi_over_2 - pi_over_4 * (offset.x / offset.y);
    }
    return r * glm::vec2(std::cos(theta), std::sin(theta));
}

static inline glm::vec3 CosineSampleHemisphere(float u1, float u2) 
{
    glm::vec2 d = SampleConcentricDisc(glm::vec2(u1, u2));
    float z = std::sqrt(std::max(0.0f, 1.0f - d.x * d.x - d.y * d.y));
    return glm::vec3(d.x, z, d.y);
}

glm::vec3 CubeUVtoCartesian(int index, glm::vec2 uv)
{
    glm::vec3 xyz(0);
    // convert range 0 to 1 to -1 to 1
    uv = uv * 2.0f - 1.0f;
    switch (index)
    {
    case 0:
        xyz = glm::vec3(1.0f, -uv.y, -uv.x);
        break;  // POSITIVE X
    case 1:
        xyz = glm::vec3(-1.0f, -uv.y, uv.x);
        break;  // NEGATIVE X
    case 2:
        xyz = glm::vec3(uv.x, 1.0f, uv.y);
        break;  // POSITIVE Y
    case 3:
        xyz = glm::vec3(uv.x, -1.0f, -uv.y);
        break;  // NEGATIVE Y
    case 4:
        xyz = glm::vec3(uv.x, -uv.y, 1.0f);
        break;  // POSITIVE Z
    case 5:
        xyz = glm::vec3(uv.x, -uv.y, -1.0f);
        break;  // NEGATIVE Z
    }
    return glm::normalize(xyz);
}

glm::vec2 CartesianToCubeUV(const glm::vec3& dir, uint32_t& faceIndex) {
    glm::vec3 absDir = glm::abs(dir); // Get absolute values for comparison
    glm::vec2 uv(0);

    if (absDir.x >= absDir.y && absDir.x >= absDir.z) {
        if (dir.x > 0) {
            // POSITIVE X (index 0)
            uv = glm::vec2(-dir.z, -dir.y) / absDir.x;
            faceIndex = 0;
        }
        else {
            // NEGATIVE X (index 1)
            uv = glm::vec2(dir.z, -dir.y) / absDir.x;
            faceIndex = 1;
        }
    }
    else if (absDir.y >= absDir.x && absDir.y >= absDir.z) {
        if (dir.y > 0) {
            // POSITIVE Y (index 2)
            uv = glm::vec2(dir.x, dir.z) / absDir.y;
            faceIndex = 2;
        }
        else {
            // NEGATIVE Y (index 3)
            uv = glm::vec2(dir.x, -dir.z) / absDir.y;
            faceIndex = 3;
        }
    }
    else {
        if (dir.z > 0) {
            // POSITIVE Z (index 4)
            uv = glm::vec2(dir.x, -dir.y) / absDir.z;
            faceIndex = 4;
        }
        else {
            // NEGATIVE Z (index 5)
            uv = glm::vec2(-dir.x, -dir.y) / absDir.z;
            faceIndex = 5;
        }
    }

    // Convert UV range from [-1, 1] to [0, 1]
    uv = uv * 0.5f + 0.5f;

    return uv;
}

// Function to sample color from a cubemap given a direction
glm::u8vec4 SampleFromCubemap(const uint8_t* cubemap, glm::vec2 uv, uint32_t dim, int index) {
    uint32_t pixelX = static_cast<uint32_t>(uv.x * ((float)dim - 1));
    uint32_t pixelY = static_cast<uint32_t>(uv.y * ((float)dim - 1));

    // Calculate the byte index for the sampled color
    uint32_t faceOffset = index * dim * dim * 4; // Assuming 4 bytes per pixel (RGB)
    uint32_t pixelIndex = faceOffset + (pixelY * dim + pixelX) * 4; // RGB

    assert(pixelIndex < (dim * dim * 6 * 4));

    // Retrieve color value from the cubemap
    glm::u8vec4 color = {
        cubemap[pixelIndex],
        cubemap[pixelIndex + 1],
        cubemap[pixelIndex + 2],
        cubemap[pixelIndex + 3],
    };

    return color; // Return the sampled color as glm::u8vec4
}

#include "math/Math.hpp"
#include "math/color/Color.hpp"
#include "glm/gtx/string_cast.hpp"

inline glm::vec4 U8Vec4ToVec4(const glm::u8vec4& u8vec) {
    return glm::vec4(
        static_cast<float>(u8vec.r) / 255.0f,
        static_cast<float>(u8vec.g) / 255.0f,
        static_cast<float>(u8vec.b) / 255.0f,
        static_cast<float>(u8vec.a) / 255.0f
    );
}

inline glm::u8vec4 Vec4ToU8Vec4(const glm::vec4& vec) {
    return glm::u8vec4(
        static_cast<unsigned char>(std::clamp(vec.r * 255.0f, 0.0f, 255.0f)),
        static_cast<unsigned char>(std::clamp(vec.g * 255.0f, 0.0f, 255.0f)),
        static_cast<unsigned char>(std::clamp(vec.b * 255.0f, 0.0f, 255.0f)),
        static_cast<unsigned char>(std::clamp(vec.a * 255.0f, 0.0f, 255.0f))
    );
}

void computeTangentBitangent(const glm::vec3& normal, glm::vec3& tangent, glm::vec3& bitangent) {
    glm::vec3 reference = (fabs(normal.y) < 0.999f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    tangent = glm::normalize(glm::cross(normal, reference));
    bitangent = glm::normalize(glm::cross(normal, tangent));
}


#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

void printProgress(float percentage) {
    int val = (int)(percentage * 100);
    int lpad = (int)(percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}

void IBLUtilsApplication::Run()
{
	Bitmap inBitMap = Bitmap(Files::Path(specs.inFile));
	NE_INFO("Loaded input file: {}, size: {}x{}", specs.inFile, inBitMap.size.x, inBitMap.size.y);
	
    const uint32_t dim = 32; // out dim of the lambertian LUT
    const uint32_t samples = 1024 * 32; // out dim of the lambertian LUT

	Bitmap outBitMap = Bitmap(glm::vec2(dim, dim * 6), 4);
    std::vector<glm::u8vec4> rgbData;

    int progress = 0;

    for (int face = 0; face < 6; ++face)
    {
        for (int row = 0; row < dim; ++row)
        {
            for (int col = 0; col < dim; ++col)
            {
                glm::vec4 irradiance(0);
                glm::vec3 N = CubeUVtoCartesian(face, glm::vec2(col / (float)dim, row / (float)dim)); // sample a direction from output UV to direction
                
                // tangent space to world
                glm::vec3 tangent, bitangent;
                computeTangentBitangent(N, tangent, bitangent);

                for (int xx = 0; xx < samples; xx++)
                {
                    // spherical to cartesian (in tangent space)
                    glm::vec3 sampleDir = CosineSampleHemisphere(Math::Random(), Math::Random()); // sample a direction from cosine given a normal
                    assert(sampleDir.y >= 0);

                    // sample direction in world space
                    glm::vec3 wsSampleDir = sampleDir.x * tangent + sampleDir.y * bitangent + sampleDir.z * N;

                    // sample from environment
                    uint32_t uvFace;
                    glm::vec2 wsUV = CartesianToCubeUV(glm::normalize(wsSampleDir), uvFace);
                    assert(wsUV.x >= 0 && wsUV.y >= 0 && wsUV.x <= 1 && wsUV.y <= 1);
                    glm::u8vec4 sampledColor = SampleFromCubemap(inBitMap.data.get(), wsUV, inBitMap.size.x, uvFace); // sample from direction from the input UV
                    glm::vec4 rgb = rgbe_to_float(sampledColor);

                    // cosine theta = dot(y, (0,1,0)) = y
                    float pdf = sampleDir.y * glm::one_over_pi<float>(); // cos theta / pi
                    auto newIrradiance = rgb / pdf;
                    irradiance += newIrradiance; // convert to u8vec4 for averaging
                }

                irradiance /= (float)samples;
                rgbData.push_back(float_to_rgbe(irradiance));

                progress++;
                printProgress((float)progress / (dim * dim * 6));
            }
        }
    }

    assert(rgbData.size() == 6 * dim * dim);

    assert(outBitMap.GetLength() == rgbData.size() * 4);
    memcpy(outBitMap.data.get(), rgbData.data(), rgbData.size() * 4);

	outBitMap.Write(Files::Path(specs.outFile, false));
}
