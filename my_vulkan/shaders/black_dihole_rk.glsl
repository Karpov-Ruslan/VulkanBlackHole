#include "black_hole.in"

#ifndef BLACK_DIHOLE_RK_GLSL
#define BLACK_DIHOLE_RK_GLSL

///////////////////////////////// Geodesic Data Section /////////////////////////////////

struct GeodesicData {
    vec4 coord; // ( x, y, z, t)
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

// Расстояние до первой дыры
float r1(vec4 pos) {
	return sqrt(pos[0]*pos[0]+pos[1]*pos[1]+(pos[2]-H1Z)*(pos[2]-H1Z));
}



// Расстояние до второй дыры

float r2(vec4 pos) {
	return sqrt(pos[0]*pos[0]+pos[1]*pos[1]+(pos[2]-H2Z)*(pos[2]-H2Z));
}


// Потенциал
float U(vec4 pos) {
	return 1.0F + H1M/r1(pos) + H2M/r2(pos);
}

// Шаг для вычисления градиента
float step(vec4 pos) {
	return GRAD_H*(1.0F+GRAD_STRETCH*(pos[0]*pos[0]+pos[1]*pos[1]+pos[2]*pos[2]));
}


// Метрика Рейсснера-Нордстрема
float metric(vec4 pos, int i, int j) {
	if ( i == j ) {
		float u = U(pos);
		u = u*u;
		if ( i == 3 )  return 1/u;
		else return -u;
	}
	return 0.0F;
}

//Обратная метрика

float invmetric(vec4 pos, int i, int j) {
	if ( i == j) {
		float u = U(pos);
		u = u * u;
		if (i == 3) return u;
		else return -1/u;
	}
	return 0.0F;
}

// Градиент метрики d_k g_{ij} для вычисления Символов Кристоффеля
// поскольку метрика симметричная, для градиента используется формула 2-го порядка:
// grad = (g(p+h)-g(p-h))/(2h)
// общий для всех градиентов знаменатель опускается (будет использован  далее)
float mgrad(vec4 pos, int i, int j, int k, float h) {
	if (k == 3 ) return 0.0F; // градиент по времени равен 0 -- метрика от времени не зависит
	if (i != j) return 0.0F; // вычисляем только для диагональных элементов
	vec4 dhp = pos;
	vec4 dhm = pos;
	dhp[k] += h;
	dhm[k] -= h;
	return metric(dhp, i, i) - metric(dhm, i, i);
}


// вычисление символов Кристоффеля (сразу по всем направлениям)
vec4 christoffelSymbol(vec4 coord, int k, int l, float h) {
	int i;
	vec4 gamma = {0.0F, 0.0F, 0.0F, 0.0F};
	for ( i = 0; i < 4; i++ ) {
		// пользуемся тем, что метрика диагональная 
		gamma[i] = invmetric(coord, i, i)*(mgrad(coord, i, k, l, h)
					+ mgrad(coord, i, l, k, h) -mgrad(coord, k, l, i, h))/4/h;
		
	}
	
	return gamma;
}

#define u(r1, r2) (1+H1M/(r1)+H2M/(r2))

#define Phi(r1_3, r2_3) (H1M/(r1_3)+H2M/(r2_3))
#define Xi(r1_3, r2_3) (H1M*H1Z/(r1_3)+H2M*H2Z/(r2_3))

// уравнение геодезической
GeodesicData geo(GeodesicData x) {
	float _r1 = r1(x.coord);
	float _r2 = r2(x.coord);
	float _u = u(_r1, _r2);
	float _r13 = pow(_r1, 3.0F);
	float _r23 = pow(_r2, 3.0F);
	float _Phi = Phi(_r13, _r23);
	float _Xi = Xi(_r13, _r23);
	float _u1 = pow(_u, -1.0F);
	float _u4 = pow(_u, -4.0F);
	float dt = x.derivative[3];
	float _u4dt2 = _u4*dt*dt;
	float _PhiX = _Phi*x.coord[0];
	float _PhiY = _Phi*x.coord[1];
	float _PhiZ = _Phi*x.coord[2] - _Xi;
	float dx = x.derivative[0];
	float dy = x.derivative[1];
	float dz = x.derivative[2];
	float _Pdx = _PhiX*dx;
	float _Pdy = _PhiY*dy;
	float _Pdz = _PhiZ*dz;
	float _Sum = _Pdx+_Pdy+_Pdz;
	GeodesicData d = {
		x.derivative,
		vec4(_u1*(-_PhiX*_u4dt2-dx*(3*_Pdx - 2*_Sum)), // dx'
			 _u1*(-_PhiY*_u4dt2-dy*(3*_Pdy - 2*_Sum)), // dy'
			 _u1*(-_PhiZ*_u4dt2-dz*(3*_Pdz - 2*_Sum)), // dz'
			 -2*_u1*dt*_Sum // dt'
			 )};
	return d;
}



// Assume theta is pi/2 and it is constant.
void transformPositionAndDirectionToGeodesic(vec3 position, vec3 direction, out GeodesicData geodesicData, out vec3 rotationAxis) {
    geodesicData.coord = vec4(position, 0.0F);
	geodesicData.derivative = vec4(normalize(direction), 1.0F);
}

void transformGeodesicToPositionAndDirection(GeodesicData geodesicData, vec3 rotationAxis, out vec3 position, out vec3 direction) {
	position[0] = geodesicData.coord[0];
	position[1] = geodesicData.coord[1];
	position[2] = geodesicData.coord[2];
	direction[0] = geodesicData.derivative[0];
	direction[1] = geodesicData.derivative[1];
	direction[2] = geodesicData.derivative[2];
}

// Explicit Runge-Kutta method (RK4 or RK2)
GeodesicData rk(GeodesicData geodesicData, float h) {

#ifdef RUNGE_KUTTE_4
    GeodesicData k1 = geo(geodesicData);
    GeodesicData k2 = geo(operatorPlus(geodesicData, operatorMultiply(k1, h*0.5F)));
    GeodesicData k3 = geo(operatorPlus(geodesicData, operatorMultiply(k2, h*0.5F)));
    GeodesicData k4 = geo(operatorPlus(geodesicData, operatorMultiply(k3, h)));
    return operatorPlus(geodesicData, operatorMultiply(operatorPlus(k1, operatorPlus(operatorMultiply(k2, 2.0F),
        operatorPlus(operatorMultiply(k3, 2.0F), k4))), 0.166666666666666666667F*h));
#endif // RUNGE_KUTTE_4

#ifdef RUNGE_KUTTE_2
    GeodesicData k1 = geo(geodesicData);
    GeodesicData k2 = geo(operatorPlus(geodesicData, operatorMultiply(k1, h)));
    return operatorPlus(geodesicData, operatorMultiply(operatorPlus(k1, k2), 0.5F*h));
#endif // RUNGE_KUTTE_2

#ifdef RUNGE_KUTTE_1
    return operatorPlus(geodesicData, operatorMultiply(geo(geodesicData), h));
#endif // RUNGE_KUTTE_1

}

#endif // BLACK_DIHOLE_RK_GLSL
