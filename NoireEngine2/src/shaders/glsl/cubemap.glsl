vec3 CubeUVtoCartesian(int index, vec2 uv) 
{
    vec3 xyz = vec3(0.0);
    uv = uv * 2.0 - 1.0;
    
    switch (index) {
        case 0:
            xyz = vec3(1.0, -uv.y, -uv.x);  // POSITIVE X
            break;
        case 1:
            xyz = vec3(-1.0, -uv.y, uv.x);  // NEGATIVE X
            break;
        case 2:
            xyz = vec3(uv.x, 1.0, uv.y);    // POSITIVE Y
            break;
        case 3:
            xyz = vec3(uv.x, -1.0, -uv.y);  // NEGATIVE Y
            break;
        case 4:
            xyz = vec3(uv.x, -uv.y, 1.0);   // POSITIVE Z
            break;
        case 5:
            xyz = vec3(uv.x, -uv.y, -1.0);  // NEGATIVE Z
            break;
    }
    
    return normalize(xyz);
}

vec2 CartesianToCubeUV(vec3 dir, out int faceIndex) 
{
    vec3 absDir = abs(dir);  // Get absolute values for comparison
    vec2 uv = vec2(0.0);

    if (absDir.x >= absDir.y && absDir.x >= absDir.z) {
        if (dir.x > 0.0) {
            // POSITIVE X (index 0)
            uv = vec2(-dir.z, -dir.y) / absDir.x;
            faceIndex = 0;
        } else {
            // NEGATIVE X (index 1)
            uv = vec2(dir.z, -dir.y) / absDir.x;
            faceIndex = 1;
        }
    } else if (absDir.y >= absDir.x && absDir.y >= absDir.z) {
        if (dir.y > 0.0) {
            // POSITIVE Y (index 2)
            uv = vec2(dir.x, dir.z) / absDir.y;
            faceIndex = 2;
        } else {
            // NEGATIVE Y (index 3)
            uv = vec2(dir.x, -dir.z) / absDir.y;
            faceIndex = 3;
        }
    } else {
        if (dir.z > 0.0) {
            // POSITIVE Z (index 4)
            uv = vec2(dir.x, -dir.y) / absDir.z;
            faceIndex = 4;
        } else {
            // NEGATIVE Z (index 5)
            uv = vec2(-dir.x, -dir.y) / absDir.z;
            faceIndex = 5;
        }
    }

    uv = uv * 0.5 + 0.5;
    return uv;
}
