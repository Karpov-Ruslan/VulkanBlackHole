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

float christoffelSymbol(uint i, uint j, uint k, GeodesicData geodesicData) {
    // Additional variables
    float r = geodesicData.coord[0];
    float theta = geodesicData.coord[1];

    if (i == 0U) {
        if (j == 0U) {
            if (k == 0U) {return -(0.5F*BLACK_HOLE_RADIUS/(r*r*(1.0F - BLACK_HOLE_RADIUS/r)));}
            if (k == 1U) {return 0.0F;}
            if (k == 2U) {return 0.0F;}
            if (k == 3U) {return 0.0F;}
        }
        if (j == 1U) {
            if (k == 0U) {return 0.0F;}
            if (k == 1U) {return 1.0F/r;}
            if (k == 2U) {return 0.0F;}
            if (k == 3U) {return 0.0F;}
        }
        if (j == 2U) {
            if (k == 0U) {return 0.0F;}
            if (k == 1U) {return 0.0F;}
            if (k == 2U) {return 1.0F/r;}
            if (k == 3U) {return 0.0F;}
        }
        if (j == 3U) {
            if (k == 0U) {return 0.0F;}
            if (k == 1U) {return 0.0F;}
            if (k == 2U) {return 0.0F;}
            if (k == 3U) {return (0.5F*BLACK_HOLE_RADIUS/(r*r*(1.0F - BLACK_HOLE_RADIUS/r)));}
        }
    }
    if (i == 1U) {
        if (j == 0U) {
            if (k == 0U) {return 0.0F;}
            if (k == 1U) {return 1.0F/r;}
            if (k == 2U) {return 0.0F;}
            if (k == 3U) {return 0.0F;}
        }
        if (j == 1U) {
            if (k == 0U) {return -r*(1.0F - BLACK_HOLE_RADIUS/r);}
            if (k == 1U) {return 0.0F;}
            if (k == 2U) {return 0.0F;}
            if (k == 3U) {return 0.0F;}
        }
        if (j == 2U) {
            if (k == 0U) {return 0.0F;}
            if (k == 1U) {return 0.0F;}
            if (k == 2U) {return 1.0F/tan(theta);}
            if (k == 3U) {return 0.0F;}
        }
        if (j == 3U) {
            if (k == 0U) {return 0.0F;}
            if (k == 1U) {return 0.0F;}
            if (k == 2U) {return 0.0F;}
            if (k == 3U) {return 0.0F;}
        }
    }
    if (i == 2U) {
        if (j == 0U) {
            if (k == 0U) {return 0.0F;}
            if (k == 1U) {return 0.0F;}
            if (k == 2U) {return 1.0F/r;}
            if (k == 3U) {return 0.0F;}
        }
        if (j == 1U) {
            if (k == 0U) {return 0.0F;}
            if (k == 1U) {return 0.0F;}
            if (k == 2U) {return 1.0F/tan(theta);}
            if (k == 3U) {return 0.0F;}
        }
        if (j == 2U) {
            if (k == 0U) {
                float valOfSin = sin(theta);
                return -r*(1.0F - BLACK_HOLE_RADIUS/r)*valOfSin*valOfSin;
            }
            if (k == 1U) {return -sin(theta)*cos(theta);}
            if (k == 2U) {return 0.0F;}
            if (k == 3U) {return 0.0F;}
        }
        if (j == 3U) {
            if (k == 0U) {return 0.0F;}
            if (k == 1U) {return 0.0F;}
            if (k == 2U) {return 0.0F;}
            if (k == 3U) {return 0.0F;}
        }
    }
    if (i == 3U) {
        if (j == 0U) {
            if (k == 0U) {return 0.0F;}
            if (k == 1U) {return 0.0F;}
            if (k == 2U) {return 0.0F;}
            if (k == 3U) {return (0.5F*BLACK_HOLE_RADIUS/(r*r*(1.0F - BLACK_HOLE_RADIUS/r)));}
        }
        if (j == 1U) {
            if (k == 0U) {return 0.0F;}
            if (k == 1U) {return 0.0F;}
            if (k == 2U) {return 0.0F;}
            if (k == 3U) {return 0.0F;}
        }
        if (j == 2U) {
            if (k == 0U) {return 0.0F;}
            if (k == 1U) {return 0.0F;}
            if (k == 2U) {return 0.0F;}
            if (k == 3U) {return 0.0F;}
        }
        if (j == 3U) {
            if (k == 0U) {return 0.5F*BLACK_HOLE_RADIUS*(1.0F - BLACK_HOLE_RADIUS/r)/(r*r);}
            if (k == 1U) {return 0.0F;}
            if (k == 2U) {return 0.0F;}
            if (k == 3U) {return 0.0F;}
        }
    }

    return 0.0F;
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
    GeodesicData ret;

    ret.coord = geodesicData.derivative;
    ret.derivative = vec4(0.0F);

    for (uint i = 0U; i < 4U; i++) {
        for (uint j = 0U; j < 4U; j++) {
            for (uint k = 0U; k < 4U; k++) {
                ret.derivative[k] += -christoffelSymbol(i, j, k, geodesicData)*geodesicData.derivative[i]*geodesicData.derivative[j];
            }
        }
    }

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