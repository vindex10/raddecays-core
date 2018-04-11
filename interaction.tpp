#include <cmath>
#include <complex>
#include <functional>
#include <gsl/gsl_sf_coupling.h>
#include <gsl/gsl_sf_bessel.h>
#include "cubature.h"
#include "utils.hpp"
#include <iostream>

using namespace std;

template <class Eq>
double Interaction<Eq>::widthMel(std::complex<double> mel) {
    double Mf = 2*outstate.eq.env.mC + outstate.eq.E;
    double k = -Mf + std::sqrt(Mf*Mf + 2*Mf*(instate.eq.E - outstate.eq.E));
    return 2.*alphaEM*k*Mf/(k+Mf)*std::norm(mel);
}

template <class Eq>
double Interaction<Eq>::coefQ(double xL, double xJ, double xlam, double xjf, double xji) {
    if (std::lround(instate.eq.xS - outstate.eq.xS) != 0) {
        return 0.;
    }

    double res = 0;
    double xs;
    for (int xs_cnt = std::lround(instate.eq.xS); xs_cnt >= -std::lround(instate.eq.xS-2.); xs_cnt-=2) {
        xs = (double)xs_cnt;
        res += clebsch(outstate.eq.xL, xjf-xs+1., outstate.eq.xS, xs, outstate.eq.xJ, xjf)*
               clebsch(instate.eq.xL, xji-xs+1., instate.eq.xS, xs, instate.eq.xJ, xji)*
               clebsch(xJ, xlam, instate.eq.xL, xji-xs+1., outstate.eq.xL, xjf-xs+1.);
    }
    res *= std::sqrt(instate.eq.xL*xL/outstate.eq.xL/4./M_PI)*clebsch(instate.eq.xL, 1., xJ, 1., outstate.eq.xL, 1.);
    
    return res;
}

template <class Eq>
double Interaction<Eq>::coefC(double xL, double xJ, double xlam, double xjf, double xji) {
    double res = 0;
    double xm2, xsq, xsqbar;
    for (int m2_cnt = 3; m2_cnt >= -1; m2_cnt-=2)
        for(int sq_cnt = 2; sq_cnt >= 0; sq_cnt-=2)
            for(int sqbar_cnt = 2; sqbar_cnt >= 0; sqbar_cnt -= 2) {
                xm2 = (double)m2_cnt;
                xsq = (double)sq_cnt;
                xsqbar = (double)sqbar_cnt;

                res += (xm2-1.)/2.*
                       clebsch(xL, xlam-xm2+1., 3., xm2, xJ, xlam)*
                       clebsch(outstate.eq.xL, xjf-(xsq+xsqbar-1.)+1., outstate.eq.xS, xsq+xsqbar-1., outstate.eq.xJ, xjf)*
                       clebsch(instate.eq.xL, xji-(xsq+xsqbar-xm2)+1., instate.eq.xS, xsq+xsqbar-xm2,instate.eq.xJ, xji)*
                       clebsch(2., xsq, 2., xsqbar, outstate.eq.xS, xsq+xsqbar-1.)*
                       clebsch(2., xsq, 2., xsqbar-xm2+1., instate.eq.xS, xsq+xsqbar-xm2)*
                       clebsch(xL, xlam-xm2+1.,instate.eq.xL, xji-(xsq+xsqbar-xm2)+1., outstate.eq.xL, xjf-(xsqbar+xsq-1.)+1.);
            }
    res *= -std::sqrt(2.)*std::sqrt(instate.eq.xL*xL/4./M_PI/outstate.eq.xL)*clebsch(instate.eq.xL, 1., xL, 1., outstate.eq.xL, 1.);
    return res;
}

template <class Eq>
int Interaction<Eq>::melMxJ_f(unsigned ndim
                     ,const double *x
                     ,void *fdata
                     ,unsigned fdim
                     ,double *fval) {
    struct melParamBundle* params = static_cast<struct melParamBundle*>(fdata);
    State<Eq>* in = &(params->obj->instate);
    State<Eq>* out = &(params->obj->outstate);

    if (std::lround((in->eq.xS + out->eq.xS + params->xJ - 3.)/2.) % 2 == 0) {
        fval[0] = 0.;
        fval[1] = 0.;
        return 0;
    }

    double k = 2*out->eq.env.mC + out->eq.E;
    k = -k + std::sqrt(k*k + 2*k*(in->eq.E - out->eq.E));

    double pC = (*in)(*x)*(*out)(*x);
    pC *= -std::sqrt(2.*M_PI)*std::sqrt(params->xJ)*k/2./(in->eq.env.muR)*(params->coefC1)*gsl_sf_bessel_jl(std::lround((params->xJ - 1.)/2.), (*x)*k/2.);

    if (std::lround((params->xJ-1.)/2.)%2 == 0) {
        fval[0] = (std::lround((params->xJ-1.)/4.)%2 == 0 ? 1. : -1.)*pC;
        fval[1] = 0.;
    } else {
        fval[0] = 0.;
        fval[1] = -(std::lround((params->xJ-3.)/4.)%2 == 0 ? 1. : -1.)*pC;
    }

    return 0;
}

template <class Eq>
std::complex<double> Interaction<Eq>::melMxJ(double xJ, double xlam, double xjf, double xji) {
    double* res = new double[2];
    double err;
    struct melParamBundle params;
    params.obj = this;
    params.xJ = xJ;
    params.coefC1 = coefC(xJ, xJ, 2.-xlam, xjf, xji);

    double minR = 0.;
    double maxR = std::min(instate.maxR, outstate.maxR);
    hcubature(2, melMxJ_f, &params, 1, &minR, &maxR, 0, 1E-5, 0, ERROR_INDIVIDUAL, res, &err);
    return std::complex<double>(res[0], res[1]);
}

template <class Eq>
int Interaction<Eq>::melMLW_f(unsigned ndim
                     ,const double *x
                     ,void *fdata
                     ,unsigned fdim
                     ,double *fval) {
    struct melParamBundle* params = static_cast<struct melParamBundle*>(fdata);
    State<Eq>* in = &(params->obj->instate);
    State<Eq>* out = &(params->obj->outstate);

    if (std::lround((in->eq.xS + out->eq.xS + params->xJ - 3.)/2.) % 2 == 0) {
        fval[0] = 0.;
        fval[1] = 0.;
        return 0;
    }

    double k = 2*out->eq.env.mC + out->eq.E;
    k = -k + std::sqrt(k*k + 2*k*(in->eq.E - out->eq.E));

    double pC = (*in)(*x)*(*out)(*x);
    pC *= -std::sqrt(2.*M_PI)*std::sqrt(3)*k/2./(in->eq.env.muR)*(params->coefC1);

    fval[0] = 0.;
    fval[1] = -pC;

    return 0;
}

template <class Eq>
std::complex<double> Interaction<Eq>::melMLW(double xlam, double xjf, double xji) {
    double* res = new double[2];
    double err;
    struct melParamBundle params;
    params.obj = this;
    params.coefC1 = coefC(3., 3., 2.-xlam, xjf, xji);

    double minR = 0.;
    double maxR = std::min(instate.maxR, outstate.maxR);
    hcubature(2, melMLW_f, &params, 1, &minR, &maxR, 0, 1E-5, 0, ERROR_INDIVIDUAL, res, &err);
    return std::complex<double>(res[0], res[1]);
}
template <class Eq>
double Interaction<Eq>::widthMxJ(double xJ, double xlam, double xjf, double xji) {
    std::complex<double> mel = 0.;
    for (int i=1; i <= std::lround((xJ-1.)/2.); ++i) {
        mel += melMxJ(2.*i+1., xlam, xjf, xji);
    }
    return widthMel(mel);
}

template <class Eq>
double Interaction<Eq>::widthMxJ(double xJ, double xlam, double xji) {
    double res = 0.;
    for (int jf_cnt = 0; jf_cnt < std::lround(outstate.eq.xJ); ++jf_cnt) {
        res += widthMxJ(xJ, xlam, -outstate.eq.xJ+2.+2.*(double)jf_cnt, xji);
    }
    return res;
}

template <class Eq>
double Interaction<Eq>::widthMxJ(double xJ, double xlam) {
    double res = 0.;
    for (int ji_cnt = 0; ji_cnt < std::lround(instate.eq.xJ); ++ji_cnt) {
        res += widthMxJ(xJ, xlam, -instate.eq.xJ+2.+2.*(double)ji_cnt);
    }
    return res/instate.eq.xJ;
}

template <class Eq>
double Interaction<Eq>::widthMxJ(double xJ) {
    return widthMxJ(xJ, -1.) + widthMxJ(xJ, 3.);
}

template <class Eq>
double Interaction<Eq>::widthMxJHel(double xJ, double xH) {
    return (widthMxJ(xJ, -1., -2.-xH) + widthMxJ(xJ, 3., 6.-xH) + ( std::lround(xH) != 1 ? widthMxJ(xJ, -1., -2.-(2.-xH)) + widthMxJ(xJ, 3., 6.-(2.-xH)) : 0.))/instate.eq.xJ;
}

template <class Eq>
int Interaction<Eq>::melExJ_f(unsigned ndim
                     ,const double *x
                     ,void *fdata
                     ,unsigned fdim
                     ,double *fval) {
    struct melParamBundle* params = static_cast<struct melParamBundle*>(fdata);
    State<Eq>* in = &(params->obj->instate);
    State<Eq>* out = &(params->obj->outstate);

    double k = 2*out->eq.env.mC + out->eq.E;
    k = -k + std::sqrt(k*k + 2*k*(in->eq.E - out->eq.E));

    fval[0] = 0.;
    fval[1] = 0.;

    if (std::lround((params->xJ - 1.)/2.) % 2 != 0) {
        double p = std::sqrt(2*M_PI)*(*in)(*x)*(*out)(*x);
        p *= (*x)*(out->eq.E - in->eq.E)*(std::sqrt((params->xJ + 1.)/2.)*gsl_sf_bessel_jl(std::lround((params->xJ-3.)/2.), k*(*x)/2.)*(params->coefQ1) - std::sqrt((params->xJ-1.)/2.)*gsl_sf_bessel_jl(std::lround((params->xJ+1.)/2.), (*x)*k/2.)*(params->coefQ2));
        fval[1] += (std::lround((params->xJ+1.)/4.)%2 == 0 ? 1.:-1.)*p;
    }

    if (std::lround((in->eq.xS + out->eq.xS + params->xJ-3.)/2.) % 2 == 0) {
        double p = std::sqrt(2*M_PI)*(*in)(*x)*(*out)(*x);
        p *= (params->xlam-1.)/2.*k/2./(in->eq.env.muR)*(std::sqrt((params->xJ + 1.)/2.)*gsl_sf_bessel_jl(std::lround((params->xJ-3.)/2.), k*(*x)/2.)*(params->coefC1) - std::sqrt((params->xJ-1.)/2.)*gsl_sf_bessel_jl(std::lround((params->xJ+1.)/2.), (*x)*k/2.)*(params->coefC2));
        if (std::lround((params->xJ + 1.)/2.)%2 == 0) {
            fval[0] += (std::lround((params->xJ+1.)/4.)%2 == 0?1.:-1.)*p;
        } else {
            fval[1] += -(std::lround((params->xJ-1.)/4.)%2 == 0?1.:-1.)*p;
        }
    }

    return 0;
}

template <class Eq>
std::complex<double> Interaction<Eq>::melExJ(double xJ, double xlam, double xjf, double xji) {
    double* res = new double[2];
    double err;
    struct melParamBundle params;
    params.obj = this;
    params.xJ = xJ;
    params.xlam = xlam;
    params.coefC1 = coefC(xJ-2., xJ, 2.-xlam, xjf, xji);
    params.coefC2 = coefC(xJ+2., xJ, 2.-xlam, xjf, xji);
    params.coefQ1 = coefQ(xJ-2., xJ, 2.-xlam, xjf, xji);
    params.coefQ2 = coefQ(xJ+2., xJ, 2.-xlam, xjf, xji);

    double minR = 0.;
    double maxR = std::min(instate.maxR, outstate.maxR);
    hcubature(2, melExJ_f, &params, 1, &minR, &maxR, 0, 1E-5, 0, ERROR_INDIVIDUAL, res, &err);
    return std::complex<double>(res[0], res[1]);
}

template <class Eq>
int Interaction<Eq>::melELW_f(unsigned ndim
                     ,const double *x
                     ,void *fdata
                     ,unsigned fdim
                     ,double *fval) {
    struct melParamBundle* params = static_cast<struct melParamBundle*>(fdata);
    State<Eq>* in = &(params->obj->instate);
    State<Eq>* out = &(params->obj->outstate);

    double k = 2*out->eq.env.mC + out->eq.E;
    k = -k + std::sqrt(k*k + 2*k*(in->eq.E - out->eq.E));

    fval[0] = 0.;
    fval[1] = 0.;

    double p = std::sqrt(2*M_PI)*(*in)(*x)*(*out)(*x);
    p *= (*x)*(out->eq.E - in->eq.E)*std::sqrt(2.)*(params->coefQ1);
    fval[1] += -p;

    if (std::lround((in->eq.xS + out->eq.xS)/2.) % 2 == 0) {
        double p = std::sqrt(2*M_PI)*(*in)(*x)*(*out)(*x);
        p *= (params->xlam-1.)/2.*k/2./(in->eq.env.muR)*std::sqrt(2.)*(params->coefC1);
        fval[0] += -p;
    }

    return 0;
}

template <class Eq>
std::complex<double> Interaction<Eq>::melELW(double xlam, double xjf, double xji) {
    double* res = new double[2];
    double err;
    struct melParamBundle params;
    params.obj = this;
    params.xlam = xlam;
    params.coefC1 = coefC(1., 3., 2.-xlam, xjf, xji);
    params.coefQ1 = coefQ(1., 3., 2.-xlam, xjf, xji);

    double minR = 0.;
    double maxR = std::min(instate.maxR, outstate.maxR);
    hcubature(2, melELW_f, &params, 1, &minR, &maxR, 0, 1E-5, 0, ERROR_INDIVIDUAL, res, &err);
    return std::complex<double>(res[0], res[1]);
}

template <class Eq>
double Interaction<Eq>::widthExJ(double xJ, double xlam, double xjf, double xji) {
    std::complex<double> mel = 0.;
    for (int i=1; i <= std::lround((xJ-1.)/2.); ++i) {
        mel += melExJ(2.*i+1., xlam, xjf, xji);
    }
    return widthMel(mel);
}

template <class Eq>
double Interaction<Eq>::widthExJ(double xJ, double xlam, double xji) {
    double res = 0.;
    for (int jf_cnt = 0; jf_cnt < std::lround(outstate.eq.xJ); ++jf_cnt) {
        res += widthExJ(xJ, xlam, -outstate.eq.xJ+2.+2.*(double)jf_cnt, xji);
    }
    return res;
}

template <class Eq>
double Interaction<Eq>::widthExJ(double xJ, double xlam) {
    double res = 0.;
    for (int ji_cnt = 0; ji_cnt < std::lround(instate.eq.xJ); ++ji_cnt) {
        res += widthExJ(xJ, xlam, -instate.eq.xJ+2.+2.*(double)ji_cnt);
    }
    return res/instate.eq.xJ;
}

template <class Eq>
double Interaction<Eq>::widthExJ(double xJ) {
    return widthExJ(xJ, -1.) + widthExJ(xJ, 3.);
}

template <class Eq>
double Interaction<Eq>::widthELW(double xlam, double xjf, double xji) {
    std::complex<double> mel = 0.;
    mel = melELW(xlam, xjf, xji);
    return widthMel(mel);
}

template <class Eq>
double Interaction<Eq>::widthELW(double xlam, double xji) {
    double res = 0.;
    for (int jf_cnt = 0; jf_cnt < std::lround(outstate.eq.xJ); ++jf_cnt) {
        res += widthELW(xlam, -outstate.eq.xJ+2.+2.*(double)jf_cnt, xji);
    }
    return res;
}

template <class Eq>
double Interaction<Eq>::widthELW(double xlam) {
    double res = 0.;
    for (int ji_cnt = 0; ji_cnt < std::lround(instate.eq.xJ); ++ji_cnt) {
        res += widthELW(xlam, -instate.eq.xJ+2.+2.*(double)ji_cnt);
    }
    return res/instate.eq.xJ;
}

template <class Eq>
double Interaction<Eq>::widthELW() {
    return widthELW(-1.) + widthELW(3.);
}

template <class Eq>
double Interaction<Eq>::widthExJHel(double xJ, double xH) {
    return (widthExJ(xJ, -1., -2.-xH) + widthExJ(xJ, 3., 6.-xH) + ( std::lround(xH) != 1 ? widthExJ(xJ, -1., -2.-(2.-xH)) + widthExJ(xJ, 3., 6.-(2.-xH)) : 0.))/instate.eq.xJ;
}

template <class Eq>
double Interaction<Eq>::width(double xJ, double xlam, double xjf, double xji) {
    std::complex<double> mel = 0.;
    for (int i=1; i <= std::lround((xJ-1.)/2.); ++i) {
        mel += melExJ(2.*i+1., xlam, xjf, xji) + melMxJ(2.*i+1., xlam, xjf, xji);
    }
    //cout << xJ << "," << xlam << "," << xjf << "," << xji << ": " << widthMel(mel) << endl;
    return widthMel(mel);
}

template <class Eq>
double Interaction<Eq>::width(double xJ, double xlam, double xji) {
    double res = 0.;
    for (int jf_cnt = 0; jf_cnt < std::lround(outstate.eq.xJ); ++jf_cnt) {
        res += width(xJ, xlam, -outstate.eq.xJ+2.+2.*(double)jf_cnt, xji);
    }
    return res;
}

template <class Eq>
double Interaction<Eq>::width(double xJ, double xlam) {
    double res = 0.;
    for (int ji_cnt = 0; ji_cnt < std::lround(instate.eq.xJ); ++ji_cnt) {
        res += width(xJ, xlam, -instate.eq.xJ+2.+2.*(double)ji_cnt);
    }
    return res/instate.eq.xJ;
}

template <class Eq>
double Interaction<Eq>::width(double xJ) {
    return width(xJ, -1.) + width(xJ, 3.);
}

template <class Eq>
double Interaction<Eq>::widthHel(double xJ, double xH) {
    return (width(xJ, -1., -2.-xH) + width(xJ, 3., 6.-xH) + ( std::lround(xH) != 1 ? width(xJ, -1., -2.-(2.-xH)) + width(xJ, 3., 6.-(2.-xH)) : 0.))/instate.eq.xJ;
}

template <class Eq>
double  Interaction<Eq>::reduceWidth(double width) {
    double Mf = 2*outstate.eq.env.mC + outstate.eq.E;
    double k = -Mf + std::sqrt(Mf*Mf + 2*Mf*(instate.eq.E - outstate.eq.E));
    return width*(k+Mf)/k/Mf/(instate.eq.E - outstate.eq.E)/(instate.eq.E - outstate.eq.E);
}
