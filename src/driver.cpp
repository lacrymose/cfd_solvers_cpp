
//======================================
// 1D Eiuler approximate Riemann sovler
#include "../include/EulerShockTube1D.h"
#include "../include/EulerUnsteady2D.h"

//======================================
// structured grids 2D class 
#include "../include/gridGen2D.h"



int main(){

    if (false){
        EulerSolver1D::driverEuler1D();
    }else if (false){
        EulerSolver2D::driverEuler2D();
    }else{
        Grid2D::driverGrid2D();
    }
    return 1;
}