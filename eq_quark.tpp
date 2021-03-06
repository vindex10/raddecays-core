#include <cmath>
#include <iostream>
#include "odeint_types.hpp"
#include "json_types.hpp"

template <typename Env>
void EqQuark<Env>::operator() (const fldarr &u, fldder &dudr, const double r) {
    double ruse = r < env.rC ? env.rC : r;
    dudr = -2.*env.muR*(E - env.Vv(r) - env.Vs(r)
            - (xL > 1.2 ? (xL*xL - 1.)/4./2./env.muR/r/r : 0.)
            - env.Vss(r, xS, xS1, xS2)
            - env.Vsl(ruse, xJ, xL, xS)
            - env.Vt(ruse, xJ, xL, xS)
            );

    // Smear everything
    //dudr[0] = u[1];
    //dudr[1] = -2*env.muR*(E - env.Vv(ruse) - env.Vs(ruse)
            //- (xL*xL - 1.)/4/2/env.muR/ruse/ruse
            //- env.Vss(ruse, xS, xS1, xS2)
            //- env.Vsl(ruse, xJ, xL, xS)
            //- env.Vt(ruse, xJ, xL, xS)
            //)*(r < env.rC ? std::pow(env.rC, (xL+1.)/2) : u[0]);

    // Trying to implement initial conditions as in 1607.04696v3
    //if (r < 1E-10) {
        //dudr[1] = -2*env.muR*(E - env.Vv(ruse) - env.Vs(ruse)
                //- (xL*xL - 1.)/4/2/env.muR/r/r
                //- env.Vss(ruse, xS, xS1, xS2)
                //- env.Vsl(ruse, xJ, xL, xS)
                //- env.Vt(ruse, xJ, xL, xS)
                //)*u[0];
    //} else {
        //dudr[1] = xL > 2.8 && xL < 3.2 ? 2 : 0;
    //}
}

template <typename Env>
void EqQuark<Env>::initU (fldarr &u, double h) {
    u[0] = std::pow(h, (xL+1.)/2.);
    u[1] = 0;

    //first derivative approximated from 1607.04696v3. u[1] = u' = [u(h)-u(0)]/h, h--> 0.
    //u[0] = 0;
    //u[1] = eq.xL < 1.2 ? 1 : 0; // xL is double, so == is bad idea
}

template <typename Env>
template <class cnt>
void EqQuark<Env>::initTu (cnt &Tu, double h) {
    operator()(fldarr(), Tu[0], h);
    Tu[0] *= std::pow(h, (xL+1.)/2.);
    Tu[1] = (xL < 3.2 && xL > 2.8 ? 2 : 0);
}

template <typename Env>
void from_json(const json &j, EqQuark<Env>& p) {
    try {
        p.xJ = j.at("xJ").get<double>();
    } catch(json::type_error& e) {
        std::cerr << e.what() << std::endl;
    } catch(json::out_of_range& e) {
        std::cerr << e.what() << std::endl;
    }

    try {
        p.xL = j.at("xL").get<double>();
    } catch(json::type_error& e) {
        std::cerr << e.what() << std::endl;
    } catch(json::out_of_range& e) {
        std::cerr << e.what() << std::endl;
    }

    try {
        p.xS = j.at("xS").get<double>();
    } catch(json::type_error& e) {
        std::cerr << e.what() << std::endl;
    } catch(json::out_of_range& e) {
        std::cerr << e.what() << std::endl;
    }

    try {
        p.xS1 = j.at("xS1").get<double>();
    } catch(json::type_error& e) {
        std::cerr << e.what() << std::endl;
    } catch(json::out_of_range& e) {
        std::cerr << e.what() << std::endl;
    }

    try {
        p.xS2 = j.at("xS2").get<double>();
    } catch(json::type_error& e) {
        std::cerr << e.what() << std::endl;
    } catch(json::out_of_range& e) {
        std::cerr << e.what() << std::endl;
    }

    try {
        p.E = j.at("E").get<double>();
    } catch(json::type_error& e) {
        std::cerr << e.what() << std::endl;
    } catch(json::out_of_range& e) {
        std::cerr << e.what() << std::endl;
    }

    try {
        p.env = j.at("env");
    } catch(json::type_error& e) {
        std::cerr << e.what() << std::endl;
    } catch(json::out_of_range& e) {
        std::cerr << e.what() << std::endl;
    }

}
