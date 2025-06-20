#include "black_hole.in"

///////////////////////////////// Geodesic Data Section /////////////////////////////////

struct GeodesicData {
    vec4 coord; // (r, theta, phi, t)
    vec4 derivative;
};

// GLSL does not support operator overloading.
GeodesicData operatorPlus(GeodesicData left, GeodesicData right) {
    GeodesicData ret = {
        left.coord + right.coord,
        left.derivative + right.derivative
    };

    return ret;
}

GeodesicData operatorMultiply(GeodesicData geodesicData, float multiplier) {
    GeodesicData ret = {
        geodesicData.coord*multiplier,
        geodesicData.derivative*multiplier
    };

    return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////// Christoffel Section //////////////////////////////////

// Christoffel Symbol is G_{ij}^{k}.
// Return value of function christoffelSymbol_i_j is christoffel symbol tensor G_{ij}^{k},
// where `i` and `j` is two last symbols in the function name.
vec4 christoffelSymbol00(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(-(0.5F*BLACK_HOLE_RADIUS/(r*r*(1.0F - BLACK_HOLE_RADIUS/r))), 0.0F, 0.0F, 0.0F);
}

vec4 christoffelSymbol01(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(0.0F, 1.0F/r, 0.0F, 0.0F);
}

vec4 christoffelSymbol02(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(0.0F, 0.0F, 1.0F/r, 0.0F);
}

vec4 christoffelSymbol03(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(0.0F, 0.0F, 0.0F, 0.5F*BLACK_HOLE_RADIUS/(r*r*(1.0F - BLACK_HOLE_RADIUS/r)));
}

vec4 christoffelSymbol10(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(0.0F, 1.0F/r, 0.0F, 0.0F);
}

vec4 christoffelSymbol11(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(-r*(1.0F - BLACK_HOLE_RADIUS/r), 0.0F, 0.0F, 0.0F);
}

vec4 christoffelSymbol12(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(0.0F, 0.0F, 1.0F/tan(theta), 0.0F);
}

vec4 christoffelSymbol13(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(0.0F);
}

vec4 christoffelSymbol20(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(0.0F, 0.0F, 1.0F/r, 0.0F);
}

vec4 christoffelSymbol21(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(0.0F, 0.0F, 1.0F/tan(theta), 0.0F);
}

vec4 christoffelSymbol22(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    float valOfSin = sin(theta);
    return vec4(-r*(1.0F - BLACK_HOLE_RADIUS/r)*valOfSin*valOfSin, -valOfSin*cos(theta), 0.0F, 0.0F);
}

vec4 christoffelSymbol23(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(0.0F);
}

vec4 christoffelSymbol30(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(0.0F, 0.0F, 0.0F, 0.5F*BLACK_HOLE_RADIUS/(r*r*(1.0F - BLACK_HOLE_RADIUS/r)));
}

vec4 christoffelSymbol31(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(0.0F);
}

vec4 christoffelSymbol32(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(0.0F);
}

vec4 christoffelSymbol33(GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];
    return vec4(0.5F*BLACK_HOLE_RADIUS*(1.0F - BLACK_HOLE_RADIUS/r)/(r*r), 0.0F, 0.0F, 0.0F);
}

/////////////////////////////////////////////////////////////////////////////////////////

// Assume theta is pi/2 and it is constant.
GeodesicData initGeodesicData(vec3 position, vec3 direction) {
    GeodesicData geodesicData;

    vec3 n_position = normalize(position);
    vec3 n_direction = normalize(direction);

    geodesicData.coord = vec4(length(position), pi/2.0F, 0.0F, 0.0F);
    geodesicData.derivative = vec4(dot(n_position, n_direction), 0.0F, length(cross(n_position, n_direction))/length(position), inversesqrt(1.0F - (BLACK_HOLE_RADIUS/geodesicData.coord[0])));

    return geodesicData;
}

// Actually, return value is derivative of `GeodesicData`, but semantically it is the same thing.
GeodesicData f(GeodesicData geodesicData) {

#define X(i, j) -(christoffelSymbol##i##j(geodesicData)*geodesicData.derivative[i]*geodesicData.derivative[j])

    GeodesicData ret = {
        geodesicData.derivative,
        X(0, 0)
        X(0, 1)
        X(0, 2)
        X(0, 3)
        X(1, 0)
        X(1, 1)
        X(1, 2)
        X(1, 3)
        X(2, 0)
        X(2, 1)
        X(2, 2)
        X(2, 3)
        X(3, 0)
        X(3, 1)
        X(3, 2)
        X(3, 3)
    };

#undef X

    return ret;
}

// Explicit Runge-Kutta method (RK4 or RK2)
// u = 1/r; r - radius
// uInfo = vec2(u, d(u)/d(phi));
GeodesicData rk(GeodesicData geodesicData, float h) {

#ifdef RUNGE_KUTTE_4
    GeodesicData k1 = f(geodesicData);
    GeodesicData k2 = f(operatorPlus(geodesicData, operatorMultiply(k1, h*0.5F)));
    GeodesicData k3 = f(operatorPlus(geodesicData, operatorMultiply(k2, h*0.5F)));
    GeodesicData k4 = f(operatorPlus(geodesicData, operatorMultiply(k3, h)));
    return operatorPlus(geodesicData, operatorMultiply(operatorPlus(k1, operatorPlus(operatorMultiply(k2, 2.0F),
        operatorPlus(operatorMultiply(k3, 2.0F), k4))), 0.166666666666666666667F*h));
#endif // RUNGE_KUTTE_4

#ifdef RUNGE_KUTTE_2
    GeodesicData k1 = f(geodesicData);
    GeodesicData k2 = f(operatorPlus(geodesicData, operatorMultiply(k1, h)));
    return operatorPlus(geodesicData, operatorMultiply(operatorPlus(k1, k2), 0.5F*h));
#endif // RUNGE_KUTTE_2

#ifdef RUNGE_KUTTE_1
    return operatorPlus(geodesicData, operatorMultiply(f(geodesicData), h));
#endif // RUNGE_KUTTE_1

}