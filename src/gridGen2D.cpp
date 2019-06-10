//*******************************************************************************
// This program generates 2D quad and triangular grids in a rectangular domain.
//
//
//      Boundary information is set up for a shock-diffraction problem
//
//                                 Wall
//                         --------------------
//     Post-shock (inflow) |                  |
//                         |->Shock           |            o: Corner node
//                         |  Mach=5.09       |
//                  .......o                  |Outflow
//                    Wall |                  |
//                         |                  |
//                         |                  |
//                         --------------------
//                               Outflow
//
// 3-Step Generation:
//
// 1. Generate a temporary structured grid data for nodes: xs(i,j) and ys(i,j)
// 2. Generate a 1D node array: x(1:nnodes), y(1:nnodes)
// 3. Generate element connectivity data: tria(1:ntria,3), quad(1:nquad,4)
//
//
// Inuput:
//        xmin, xmax = x-coordinates of the left and right ends
//        ymin, ymax = y-coordinates of the bottom and top ends
//                nx = number of nodes in x-direction
//                ny = number of nodes in y-direction
//
//        (NOTE: All input parameters are defined inside the program.)
//
// Output:
//        tria_grid_tecplot.dat = tecplot file of the triangular grid
//        quad_grid_tecplot.dat = tecplot file of the quadrilateral grid
//                    tria.grid = triangular-grid file for EDU2D-Euler solver
//                    quad.grid = quadrilateral-grid file for EDU2D-Euler solver
//                project.bcmap = file that contains boundary condition info
//
//        (NOTE: The grid file format is specific to the EDU2D-Euler solver.)
//
//        (NOTE: The grid file contains 5 boundary segments which is designed
//               for the shock-diffraction problem. See write_grid_file() on the
//               bottom of this file. Boudnary condition files project.bcmap
//               specify the boundary condition to be applied to each segment.)
//
//
//
//        written by Dr. Katate Masatsuka (info[at]cfdbooks.com),
//
// the author of useful CFD books, "I do like CFD" (http://www.cfdbooks.com).
//
// This is Version 0 (December 2011).
// This F90 code is written and made available for an educational purpose as well
// as for generating grids for the EDU2D-Euler code.
// This file may be updated in future.
//
// Katate Masatsuka, January 2012. http://www.cfdbooks.com
// translated by Luke McCulloch June 2019 LukeMcCulloch@github.com
//*******************************************************************************

//#define CHECKPT {printf("Checkpoint: .s, line .d\n",__FILE__,__LINE__);\
//fflush(stdout);}
#ifdef DEBUG_BUILD
#  define DEBUG(x) fprintf(stderr, x)
#else
#  define DEBUG(x) do {} while (0)
#endif

//=================================
#include <iostream>     // std::cout, std::fixed
#include <fstream>      // write to file
#include <iomanip>    // std::setprecision - only works for output :(
#include <math.h>       // sqrt
//=================================
#include <cstring> //needed for memset
#include <string.h>

//======================================
// my simple array class template (type)
#include "../include/tests_array.hpp"
#include "../include/array_template.hpp"
#include "../include/arrayops.hpp"

//======================================
// my simple vector class template
#include "../include/vector.h"

//======================================
using namespace std;

//======================================
//fwd declarations
struct cell_data;
class gridGen2D;



// use an array of structs (may be inefficient//)
struct cell_data{
    float xc;  // Cell-center coordinate
    Array2D<float> u  = Array2D<float>(3,1);  // Conservative variables = [rho, rho*u, rho*E]
    Array2D<float> u0 = Array2D<float>(3,1);  // Conservative variables at the previous time step
    Array2D<float> w  = Array2D<float>(3,1);  // Primitive variables = [rho, u, p]
    Array2D<float> dw = Array2D<float>(3,1);  // Slope (difference) of primitive variables
    Array2D<float> res= Array2D<float>(3,1);  // Residual = f_{j+1/2) - f_{j-1/2)
};

class gridGen2D{

public:

    //constructor
    gridGen2D();
    // explicit constructor declaring size nrow,ncol:
    explicit gridGen2D(int nx, int ny):
                    nx(nx), ny(ny){
        build();
    }
    //destructor
    ~gridGen2D();

    void build();

    //output
    void output();

    //Input  - domain size and grid dimensions
    float xmin, xmax;            //Minimum x and Max x.
    float ymin, ymax;            //Minimum y and Max y

    const float  zero = 0.0;    // minimum x and max x
    const float   one = 1.0;    // minimum y and max y
    int nx;                     // number of nodes in the x-direction
    int ny;                     // number of nodes in the y-direction


    //Output - grid files
    // tria_grid_tecplot.dat
    // quad_grid_tecplot.dat
    // tria.dat
    // quad.dat
    // project.dat


    // structured grid data
    Array2D<float>*  xs;  //Slopes between j and j-1, j and j+1
    Array2D<float>*  ys;  //Slopes between j and j-1, j and j+1


    //Local variables
    int nnodes; //Total number of nodes
    int  ntria; //Total number of triangles
    int  nquad; //Total number of quadrilaterals
    int  inode; //Local variables used in the 1D nodal array


    Array2D<int>* tria;      //Triangle connectivity data
    Array2D<int>* quad;      //Quad connectivity data
    Array2D<float>*  x;   //Nodal x coordinates, 1D array
    Array2D<float>*  y;   //Nodal y coordinates, 1D array

    float dx; //Uniform grid spacing in x-direction = (xmax-xmin)/nx
    float dy; //Uniform grid spacing in y-direction = (ymax-ymin)/ny
    int i, j, os;
};
//
gridGen2D::~gridGen2D(){
    printf("destruct");
    delete xs;
    delete ys;
    // delete tria;
    // delete quad;
    // delete x;
    // delete y;

}

//
// Default Constructor
gridGen2D::gridGen2D(){

    //  Define the domain: here we define a unit square.
    xmin = zero;
    xmax = one;

    ymin = zero;
    ymax = one;

    //  Define the grid size: the number of nodes in each direction.
    //  NOTE: "ny" is better to be an odd number to place a node at the midpoint
    //        on the left boundary, which will be a corner node in shock diffraction problem.
    nx = 401;
    ny = 401;
    build();
}
// Default Constructor
void gridGen2D::build(){//build

    // structured grid data
    xs = new Array2D<float>(nx,ny);  //Slopes between j and j-1, j and j+1
    ys = new Array2D<float>(nx,ny);  //Slopes between j and j-1, j and j+1

    //--------------------------------------------------------------------------------
    // 1. Generate a structured 2D grid data, (i,j) data: go up in y-direction//
    //
    // j=5 o--------o--------o--------o--------o
    //     |        |        |        |        |
    //     |        |        |        |        |   On the left is an example:
    //     |        |        |        |        |         nx = 5
    // j=4 o--------o--------o--------o--------o         ny = 5
    //     |        |        |        |        |
    //     |        |        |        |        |    +y
    //     |        |        |        |        |    |
    // j=3 o--------o--------o--------o--------o    |
    //     |        |        |        |        |    |____ +x
    //     |        |        |        |        |
    //     |        |        |        |        |
    // j=2 o--------o--------o--------o--------o
    //     |        |        |        |        |
    //     |        |        |        |        |
    //     |        |        |        |        |
    // j=1 o--------o--------o--------o--------o
    //     i=1      i=2      i=3      i=4      i=5

    printf("Generating structured data...");

    //  Compute the grid spacing in x-direction
    dx = (xmax-xmin)/float(nx-1);

    // //  Compute the grid spacing in y-direction
    dy = (ymax-ymin)/float(ny-1);

    //  Generate nodes in the domain.


    for (int j=0; j<ny; ++j) {       // Go up in y-direction.
        for (int i=0; i<nx; ++i) {   // Go to the right in x-direction.
        printf("\ni = %d, j = %d",i,j);
        (*xs)(i,j) = xmin + dx*float(i-1);
        (*ys)(i,j) = ymin + dy*float(j-1);

        }
    }

}


//********************************************************************************
//* Write output data file
//*
//* ------------------------------------------------------------------------------
//* Output:  Data file "solution.dat" containing for each cell the following:
//*          cell-center coordinate, density, velocity, pressure, entropy
//* ------------------------------------------------------------------------------
//*
//********************************************************************************
    void gridGen2D::output(){

    float entropy;

    // ofstream outfile;
    // outfile.open ("tria_grid_tecplot.dat");
    // for (int i=1; i<ncells+1; ++i){
    //     entropy = log( cell[i].w(2)* pow(cell[i].w(0) , (-gamma)) ) / (gamma-one);
    //     outfile << std::setprecision(16) << cell[i].xc << '\t'
    //             << std::setprecision(16) << cell[i].w(0) << '\t'
    //             << std::setprecision(16) << cell[i].w(1) << '\t'
    //             << std::setprecision(16) << cell[i].w(2) << '\t'
    //             << std::setprecision(16) << entropy <<  "\n";
    // }
    // outfile.close();

}
//--------------------------------------------------------------------------------



void driverGrid2D(){
    gridGen2D Grid;
    printf("\nGridding Complete\n");
    return;
}