#include "black_hole.in"

// u = 1/r; r - radius
// uInfo = vec2(u, d(u)/d(phi));
vec2 f(vec2 uInfo) {
    return vec2(uInfo.y, 1.5F*BLACK_HOLE_RADIUS*uInfo.x*uInfo.x - uInfo.x);
}

// Explicit Runge-Kutta method (RK4 or RK2)
// u = 1/r; r - radius
// uInfo = vec2(u, d(u)/d(phi));
vec2 rk(vec2 uInfo, float h) {

#ifdef RUNGE_KUTTE_4
    vec2 k1 = f(uInfo);
    vec2 k2 = f(uInfo + k1*h*0.5F);
    vec2 k3 = f(uInfo + k2*h*0.5F);
    vec2 k4 = f(uInfo + k3*h);
    return uInfo + 0.166666666666666666667F*h*(k1 + 2.0F*k2 + 2.0F*k3 + k4);
#endif // RUNGE_KUTTE_4

#ifdef RUNGE_KUTTE_2
    vec2 k1 = f(uInfo);
    vec2 k2 = f(uInfo + k1*h);
    return uInfo + 0.5F*h*(k1 + k2);
#endif // RUNGE_KUTTE_2

#ifdef RUNGE_KUTTE_1
    return uInfo + h*f(uInfo);
#endif // RUNGE_KUTTE_1

}