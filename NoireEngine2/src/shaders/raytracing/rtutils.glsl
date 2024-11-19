#include "../glsl/pbr.glsl"

// computes a normalized refraction direction T and returns true 
// only when total internal reflection DOES NOT occur.
// Assumes I is normalized.
// -- from Ray Tracing Gems II: 107
bool TransmissionDirection(float n1, float n2, vec3 I, vec3 N, out vec3 T)
{
    float eta = n1 / n2; // relative index of refraction
    float c1 = -dot(I, N); // cos(theta1)
    float w = eta * c1; 
    float c2m = (w - eta) * (w + eta); // cos^2(theta2) - 1
    if (c2m < -1.0)
        return false;
    T = eta * I + (w - sqrt(1.0 + c2m)) * N;
    return true;
}