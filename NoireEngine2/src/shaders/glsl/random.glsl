// generates a random number from pair of coordinates
float random (vec2 xy) {
    return fract(sin(dot(xy, vec2(12.9898,78.233)))* 43758.5453123);
}