#include "types.hpp"

template <typename Eq>
class EigenVals {
public:
    Eq eq;
    double cutscale, intstep;
    double etol, estep;
    fldarr u;
    stepper stpr;

    // coefficient near positive exponent on u-asymptotics
    double f();
 
    // find minimum of coefficient with gradient descent
    void findmin();
};

#include "eigenvals.tpp"
