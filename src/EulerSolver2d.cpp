
//======================================
// eigen for fast matrix ops
#include "GetEigen.h" //since you overload real in your code, you must include Eigen first

//======================================
// my simple array class template (type)
#include "tests_array.hpp"
#include "array_template.hpp"
#include "arrayops.hpp"



// //======================================
// // 2D Euler sovler
// 2D Eiuler approximate Riemann sovler
#include "EulerUnsteady2D.h"


//======================================
#include "EulerUnsteady2D_basic_package.h"

//======================================
// string trimfunctions
#include "StringOps.h"


//using Eigen::Dynamic;
using Eigen::MatrixXd;

// EulerSolver2D::MainData2D::MainData2D() {


//    M_inf             = 0.0;                  // Freestream Mach number to be set in the function
//                                              //    -> "initial_solution_shock_diffraction"
//                                              //    (Specify M_inf here for other problems.)
//    gamma             = 1.4;                  // Ratio of specific heats
//    CFL               = 0.95;                 // CFL number
//    t_final           = 0.18;                 // Final time to stop the calculation.
//    time_step_max     = 5000;                 // Max time steps (just a big enough number)
//    inviscid_flux     = "rhll";               // = Rotated-RHLL      , "roe"  = Roe flux
//    limiter_type      = "vanalbada";          // = Van Albada limiter, "none" = No limiter
//    nq        = 4;                    // The number of equtaions/variables in the target equtaion.
//    gradient_type     = "linear";             // or "quadratic2 for a quadratic LSQ.
//    gradient_weight   = "none";               // or "inverse_distance"
//    gradient_weight_p =  EulerSolver2D::one;  // or any other real value
// }




// EulerSolver2D::MainData2D::~MainData2D() {
//     delete[] node;
//     }

EulerSolver2D::Solver::Solver(){}


EulerSolver2D::Solver::~Solver(){
   printf("destruct Solver");
}


//********************************************************************************
//********************************************************************************
//********************************************************************************
//* Euler solver: Node-Centered Finite-Volume Method (Edge-Based)
//*
//* - Node-centered finite-volume method for unstructured grids(quad/tri/mixed)
//* - Roe flux with an entropy fix and Rotated-RHLL flux
//* - Reconstruction by unweighted least-squares method (2x2 system for gradients)
//* - Van Albada slope limiter to the primitive variable gradients
//* - 2-Stage Runge-Kutta time-stepping
//*
//********************************************************************************
void EulerSolver2D::Solver::euler_solver_main(EulerSolver2D::MainData2D& E2Ddata ){

   // //Local variables
   Array2D<real> res_norm(4,3); //Residual norms(L1,L2,Linf)
   Array2D<real>*  u0;       //Saved solution
   real dt, time;    //Time step and actual time
   int i_time_step; //Number of time steps
   int i;

   // // Allocate the temporary solution array needed for the Runge-Kutta method.
   // allocate(u0(nnodes,4));

   // These parameters are set in main. Here just print them on display.
   cout << " \n";
   cout << "Calling the Euler solver...";
   cout << " \n";
   cout << "                  M_inf = " <<  E2Ddata.M_inf << " \n";
   cout << "                    CFL = " <<  E2Ddata.CFL << " \n";
   cout << "             final time = " <<  E2Ddata.t_final << " \n";
   cout << "          time_step_max = " <<  E2Ddata.time_step_max << " \n";
   cout << "          inviscid_flux = " <<  trim(E2Ddata.inviscid_flux) << " \n";
   cout << "           limiter_type = " <<  trim(E2Ddata.limiter_type) << " \n";
   cout << " \n";

   //--------------------------------------------------------------------------------
   // First, make sure that normal mass flux is zero at all solid boundary nodes.
   // NOTE: Necessary because initial solution may generate the normal component.
   //--------------------------------------------------------------------------------

    eliminate_normal_mass_flux(E2Ddata);

   //--------------------------------------------------------------------------------
   // Time-stepping toward the final time
   //--------------------------------------------------------------------------------


}
//********************************************************************************
// End of program
//********************************************************************************




//********************************************************************************
//* Prepararion for Tangency condition (slip wall):
//*
//* Eliminate normal mass flux component at all solid-boundary nodes at the
//* beginning. The normal component will never be changed in the solver: the
//* residuals will be constrained to have zero normal component.
//*
//********************************************************************************
void EulerSolver2D::Solver::eliminate_normal_mass_flux( EulerSolver2D::MainData2D& E2Ddata ) {

   int i, j, inode;
   real normal_mass_flux;
   Array2D<real> n12(2,1);

   //  bc_loop : loop nbound
   for (size_t i = 0; i < E2Ddata.nbound; i++) {

      // only_slip_wall : if (trim(bound(i)%bc_type) == "slip_wall") then
      if ( trim(E2Ddata.bound[i].bc_type) == "slip_wall" ) {

         cout << " Eliminating the normal momentum on slip wall boundary " << i << " \n";

         // bnodes_slip_wall : loop bound(i)%nbnodes
         for (size_t j = 0; j < E2Ddata.bound[i].nbnodes; j++ ) {

            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // THIS IS A SPECIAL TREATMENT FOR SHOCK DIFFRACTION PROBLEM.
            //
            // NOTE: This is a corner point between the inflow boundary and
            //       the lower-left wall. Enforce zero y-momentum, which is
            //       not ensured by the standard BCs.
            //       This special treatment is necessary because the domain
            //       is rectangular (the left boundary is a straight ine) and
            //       the midpoint node on the left boundary is actually a corner.
            //
            //       Our computational domain:
            //
            //                 ---------------
            //          Inflow |             |
            //                 |             |  o: Corner node
            //          .......o             |
            //            Wall |             |  This node is a corner.
            //                 |             |
            //                 ---------------
            //
            //       This is to simulate the actual domain shown below:
            //      
            //         -----------------------
            // Inflow  |                     |
            //         |                     |  o: Corner node
            //         --------o             |
            //            Wall |             |
            //                 |             |
            //                 ---------------
            //      In effect, we're simulating this flow by a simplified
            //      rectangular domain (easier to generate the grid).
            //      So, an appropriate slip BC at the corner node needs to be applied,
            //      which is "zero y-momentum", and that's all.
            //
            if (i==1 and j==0) {
               inode                       = (*E2Ddata.bound[i].bnode)(j);
               (*E2Ddata.node[inode].u)(1) = zero;                                  // Make sure zero y-momentum.
               (*E2Ddata.node[inode].w)    = u2w( (*E2Ddata.node[inode].u) , E2Ddata );// Update primitive variables
               
               continue; // cycle bnodes_slip_wall // That's all we neeed. Go to the next.

            }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            inode = (*E2Ddata.bound[i].bnode)(j);
            n12(0) = (*E2Ddata.bound[i].bnx)(j);
            n12(1) = (*E2Ddata.bound[i].bny)(j);

            normal_mass_flux = (*E2Ddata.node[inode].u)(0)*n12(0) + (*E2Ddata.node[inode].u)(1)*n12(0); //tlm fixed

            (*E2Ddata.node[inode].u)(1) = (*E2Ddata.node[inode].u)(1) - normal_mass_flux * n12(0);
            (*E2Ddata.node[inode].u)(2) = (*E2Ddata.node[inode].u)(2) - normal_mass_flux * n12(1);

            (*E2Ddata.node[inode].w)    = u2w( (*E2Ddata.node[inode].u) , E2Ddata );

         }//end loop bnodes_slip_wall

         cout << " Finished eliminating the normal momentum on slip wall boundary " << i << " \n";

      }//end if only_slip_wall

   }//end loop bc_loop

 } // end eliminate_normal_mass_flux

//--------------------------------------------------------------------------------




//********************************************************************************
//* Initial solution for the shock diffraction problem:
//*
//* NOTE: So, this is NOT a general purpose subroutine.
//*       For other problems, specify M_inf in the main program, and
//*       modify this subroutine to set up an appropriate initial solution.
//
//  Shock Diffraction Problem:
//
//                             Wall
//                     --------------------
// Post-shock (inflow) |                  |
// (rho,u,v,p)_inf     |->Shock (M_shock) |            o: Corner node
//    M_inf            |                  |
//              .......o  Pre-shock       |Outflow
//                Wall |  (rho0,u0,v0,p0) |
//                     |                  |
//                     |                  |
//                     --------------------
//                           Outflow
//
//********************************************************************************
void EulerSolver2D::Solver::initial_solution_shock_diffraction(
                                             EulerSolver2D::MainData2D& E2Ddata ) {

   //Local variablesinitial_solution_shock_diffraction
   int i;
   real M_shock, u_shock, rho0, u0, v0, p0;
   real gamma = E2Ddata.gamma;

   //loop nnodes
   for (size_t i = 0; i < E2Ddata.nnodes; i++) {

      // Pre-shock state: uniform state; no disturbance has reahced yet.

      rho0  = one;
      u0    = zero;
      v0    = zero;
      p0    = one/gamma;

   // Incoming shock speed

      M_shock = 5.09;
      u_shock = M_shock * sqrt(gamma*p0/rho0);

      // Post-shock state: These values will be used in the inflow boundary condition.
       E2Ddata.rho_inf = rho0 * (gamma + one)*M_shock*M_shock/( (gamma - one)*M_shock*M_shock + two );
         E2Ddata.p_inf =   p0 * (   two*gamma*M_shock*M_shock - (gamma - one) )/(gamma + one);
         E2Ddata.u_inf = (one - rho0/E2Ddata.rho_inf)*u_shock;
         E2Ddata.M_inf = E2Ddata.u_inf / sqrt(gamma*E2Ddata.p_inf/E2Ddata.rho_inf);
         E2Ddata.v_inf = zero;

      // Set the initial solution: set the pre-shock state inside the domain.

      E2Ddata.node[i].w[0] = rho0;
      E2Ddata.node[i].w[0] = u0;
      E2Ddata.node[i].w[0] = v0;
      E2Ddata.node[i].w[0] = p0;
      (*E2Ddata.node[i].u) = w2u( (*E2Ddata.node[i].w), E2Ddata);

   }
 }  // end function initial_solution_shock_diffraction




//********************************************************************************
//* Compute U from W
//*
//* ------------------------------------------------------------------------------
//*  Input:  w =    primitive variables (rho,     u,     v,     p)
//* Output:  u = conservative variables (rho, rho*u, rho*v, rho*E)
//* ------------------------------------------------------------------------------
//* 
//********************************************************************************
Array2D<real>  EulerSolver2D::Solver::w2u(const Array2D<real>& w,
                                             EulerSolver2D::MainData2D& E2Ddata) {
   Array2D<real> u(4,1);

   u(0) = w(0);
   u(1) = w(0)*w(1);
   u(2) = w(0)*w(2);
   u(3) = w(3)/(E2Ddata.gamma-one)+half*w(1)*(w(2)*w(2)+w(3)*w(3));

   return u;

} // end function w2u
//--------------------------------------------------------------------------------

//********************************************************************************
//* Compute U from W
//*
//* ------------------------------------------------------------------------------
//*  Input:  u = conservative variables (rho, rho*u, rho*v, rho*E)
//* Output:  w =    primitive variables (rho,     u,     v,     p)
//* ------------------------------------------------------------------------------
//* 
//********************************************************************************
Array2D<real>  EulerSolver2D::Solver::u2w(const Array2D<real>& u,
                                             EulerSolver2D::MainData2D& E2Ddata) {

   Array2D<real> w(4,1);

   w(0) = u(0);
   w(1) = u(1)/u(0);
   w(2) = u(2)/u(0);
   w(3) = (E2Ddata.gamma-one)*( u(3) - half*w(0)*(w(1)*w(1)+w(2)*w(2)) );

   return w;

}//end function u2w
//--------------------------------------------------------------------------------



void EulerSolver2D::Solver::compute_lsq_coeff_nc(EulerSolver2D::MainData2D& E2Ddata) {

      int i, in, ell, ii, k;

      cout << " \n";
      cout << " " "Constructing LSQ coefficients...\n";

      // 1. Coefficients for the linear LSQ gradients

      cout << "---(1) Constructing Linear LSQ coefficients...\n";

      // nnodes
      for (size_t i = 0; i < E2Ddata.nnodes; i++) {

         //my_alloc_p2_ptr(node[i].lsq2x2_cx,node[i].nnghbrs)
         E2Ddata.node[i].lsq2x2_cx = new Array2D<real>( E2Ddata.node[i].nnghbrs, 1 );
         //my_alloc_p2_ptr(node[i].lsq2x2_cy,node[i].nnghbrs)
         E2Ddata.node[i].lsq2x2_cy = new Array2D<real>( E2Ddata.node[i].nnghbrs, 1 );
         lsq01_2x2_coeff_nc(E2Ddata, i);

      }


// 2. Coefficients for the quadratic LSQ gradients (two-step method)

   cout << "---(2) Constructing Quadratic LSQ coefficients...\n";

   //loop nnodes
   for (size_t i = 0; i < E2Ddata.nnodes; i++) {

      ii = 0;
      //loop node[i].nnghbrs;
      for (size_t k = 0; k < E2Ddata.node[i].nnghbrs; k++) {
         in = (*E2Ddata.node[i].nghbr)(k);
         //loop E2Ddata.node[in].nnghbrs;
         for (size_t ell = 0; ell < E2Ddata.node[in].nnghbrs; ell++) {
            ii = ii + 1;
         }//nghbr_nghbr
      }//nghbr

   //   call my_alloc_p2_ptr(node(i)%lsq5x5_cx, ii)
      E2Ddata.node[i].lsq5x5_cx = new Array2D<real>(ii,1);
   //   call my_alloc_p2_ptr(node(i)%lsq5x5_cy, ii)
      E2Ddata.node[i].lsq5x5_cy = new Array2D<real>(ii,1);
   //   call my_alloc_p2_ptr(node(i)%dx,node(i)%nnghbrs)
      //E2Ddata.node[i].dx = new Array2D<real>(E2Ddata.node[i].nnghbrs+1,1);
      E2Ddata.node[i].dx = new Array2D<real>(E2Ddata.node[i].nnghbrs,1);
   //   call my_alloc_p2_ptr(node(i)%dy,node(i)%nnghbrs)
      //E2Ddata.node[i].dy = new Array2D<real>(E2Ddata.node[i].nnghbrs+1,1);
      E2Ddata.node[i].dy = new Array2D<real>(E2Ddata.node[i].nnghbrs,1);
   //   call my_alloc_p2_matrix_ptr(node(i)%dw, nq,node(i)%nnghbrs)
      //E2Ddata.node[i].dw = new Array2D<real>(E2Ddata.nq, E2Ddata.node[i].nnghbrs+1);
      E2Ddata.node[i].dw = new Array2D<real>(E2Ddata.nq, E2Ddata.node[i].nnghbrs);

   }//end do

   lsq02_5x5_coeff2_nc(E2Ddata);

} //  end compute_lsq_coeff_nc

// //********************************************************************************
// //* This subroutine verifies the implementation of LSQ gradients.
// //*
// //* 1. Check if the linear LSQ gradients are exact for linear functions.
// //* 2. Check if the quadratic LSQ gradients are exact for quadratic functions.
// //*
// //* Note: Here, we use only the first component of u=(u1,u2,u3), i.e., ivar=1.
// //*
// //********************************************************************************
void EulerSolver2D::Solver::check_lsq_coeff_nc(EulerSolver2D::MainData2D& E2Ddata) {
   // use edu2d_constants   , only : p2, one, two
   // use edu2d_my_main_data, only : nnodes, node

   int       i, ix, iy, ivar;
   std::string grad_type_temp;
   real error_max_wx, error_max_wy, x, y;
   real x_max_wx, y_max_wx, x_max_wy, y_max_wy, wx, wxe, wy, wye;
   real a0, a1, a2, a3, a4, a5;

   ix = 0;
   iy = 1;

   // We only use w(0) for this test.
   ivar = 0;

//---------------------------------------------------------------------
// 1. Check linear LSQ gradients
//---------------------------------------------------------------------
  cout << " \n";
  cout << "---------------------------------------------------------\n";
  cout << "---------------------------------------------------------\n";
  cout << "- Checking Linear LSQ gradients..." << endl;

//  (1). Store a linear function in w(ivar) = x + 2*y.
//       So the exact gradient is grad(w(ivar)) = (0,1).

   cout << "- Storing a linear function values... \n";
   // nnodes
   for (size_t i = 0; i < E2Ddata.nnodes; i++) {
      x = E2Ddata.node[i].x;
      y = E2Ddata.node[i].y;
      (*E2Ddata.node[i].w)(ivar) = one*x + two*y;
   }

//  (2). Compute the gradient by linear LSQ

   cout << "- Computing linear LSQ gradients..\n";
   grad_type_temp = "linear";
   //cout << " grad_type_temp =  " << grad_type_temp << endl;
   compute_gradient_nc(E2Ddata, ivar, grad_type_temp);
   cout << " ivar = " << ivar << endl;

//  (3). Compute the relative errors (L_infinity)

   cout << "- Computing the relative errors (L_infinity)..\n";
   error_max_wx = -one;
   error_max_wy = -one;


   //loop nnodes
   for (size_t i = 0; i < E2Ddata.nnodes; i++) {
      //cout << " (*E2Ddata.node[i].gradw)(ivar,ix) = " << (*E2Ddata.node[i].gradw)(ivar,ix) << endl;
      //cout << " (*E2Ddata.node[i].gradw)(ivar,iy) = " << (*E2Ddata.node[i].gradw)(ivar,iy) << endl;
      error_max_wx = max( std::abs( (*E2Ddata.node[i].gradw)(ivar,ix) - one )/one, error_max_wx );
      error_max_wy = max( std::abs( (*E2Ddata.node[i].gradw)(ivar,iy) - two )/two, error_max_wy );
   }

   cout << " Max relative error in wx =  " << error_max_wx << "\n";
   cout << " Max relative error in wy =  " << error_max_wy << "\n";
   cout << "---------------------------------------------------------\n";
   cout << "---------------------------------------------------------\n";


//---------------------------------------------------------------------
// 2. Check quadratic LSQ gradients
//---------------------------------------------------------------------
   cout << "\n";
   cout << "---------------------------------------------------------\n";
   cout << "---------------------------------------------------------\n";
   cout << "- Checking Quadratic LSQ gradients...\n";

//  (1). Store a quadratic function in w(ivar) = a0 + a1*x + a2*y + a3*x**2 + a4*x*y + a5*y**2
//       So the exact gradient is grad(w(ivar)) = (a1+2*a3*x+a4*y, a2+2*a5*y+a4*x)

   a0 =    21.122;
   a1 =     1.000;
   a2 = -   1.970;
   a3 =   280.400;
   a4 = -2129.710;
   a5 =   170.999;

   cout << "- Storing a quadratic function values...\n";
   // loop nnodes
   for (size_t i = 0; i < E2Ddata.nnodes; i++) {
      x = E2Ddata.node[i].x;
      y = E2Ddata.node[i].y;
      (*E2Ddata.node[i].w)(ivar) = a0 + a1*x + a2*y + a3*x*x + a4*x*y + a5*y*y;
   }

//  (2). Compute the gradient by linear LSQ

   cout << "- Computing quadratic LSQ gradients..\n";
   grad_type_temp = "quadratic2";
   compute_gradient_nc(E2Ddata,ivar,grad_type_temp);

//  (3). Compute the relative errors (L_infinity)

   cout << "- Computing the relative errors (L_infinity)..\n";
   error_max_wx = -one;
   error_max_wy = -one;
   // find node i's of max grad error
   size_t intmax_x = 0;
   size_t intmax_y = 0;
   //loop nnodes
   for (size_t i = 0; i < E2Ddata.nnodes; i++) {
      x = E2Ddata.node[i].x;
      y = E2Ddata.node[i].y;

      if ( std::abs( (*E2Ddata.node[i].gradw)(ivar,ix) - 
            (a1+2.0*a3*x+a4*y) )/(a1+2.0*a3*x+a4*y) > error_max_wx )  {
         wx  = (*E2Ddata.node[i].gradw)(ivar,ix);
         wxe = a1 + 2.0*a3*x + a4*y;
         error_max_wx = std::abs( wx - wxe )/wxe;
         x_max_wx = x;
         y_max_wx = y;
         intmax_x = i;
      }

      if ( std::abs( (*E2Ddata.node[i].gradw)(ivar,iy) - 
            (a2+2.0*a5*y+a4*x) )/(a2+2.0*a5*y+a4*x) > error_max_wy )  {
         wy  = (*E2Ddata.node[i].gradw)(ivar,iy);
         wye = a2 + 2.0*a5*y + a4*x;
         error_max_wy = std::abs( wy - wye )/wye;
         x_max_wy = x;
         y_max_wy = y;
         intmax_y = i;
      }

   }//end do

  cout << " Max relative error in wx = " <<  error_max_wx <<  " at (x,y) = (" 
                                          <<  x_max_wx << " , " << y_max_wx << ")\n";
  cout << "   At this location, LSQ ux = " <<  wx <<  ": Exact ux = " <<  wxe << "\n";
  cout << " Max relative error in wy = " <<  error_max_wy <<  " at (x,y) = (" 
                                          <<  x_max_wy << " , " << y_max_wy << ")\n";
  cout << "   At this location, LSQ uy = " <<  wy <<  ": Exact uy = " <<  wye << "\n";
  cout << "---------------------------------------------------------\n";
  cout << "---------------------------------------------------------\n";
  cout << " " << endl;


} //  end check_lsq_coeff_nc
//********************************************************************************
//*
//********************************************************************************




//********************************************************************************
//* This subroutine computes gradients at nodes for the variable u(ivar),
//* where ivar = 1,2,3, ..., or nq.
//*
//* ------------------------------------------------------------------------------
//*  Input: node[:).u(ivar)
//*
//* Output: node[i].gradu(ivar,1:2) = ( du(ivar)/dx, du(ivar)/dy )
//* ------------------------------------------------------------------------------
//********************************************************************************
void EulerSolver2D::Solver::compute_gradient_nc(
                              EulerSolver2D::MainData2D& E2Ddata,
                              int ivar, std::string grad_type) {

   //use edu2d_my_main_data, only : node, nnodes

   //integer, intent(in) :: ivar
   //std::string grad_type

   int in;

   cout << "computing gradient nc type " << grad_type << endl;

   if (trim(grad_type) == "none") {
      cout << "trim(grad_type) == none, return " << endl;
      return;
   }
   //   else {
   //      cout << "trim(grad_type) == " << trim(grad_type) << " full =  " << grad_type << endl;
   //   }

   //-------------------------------------------------
   //  Two-step quadratic LSQ 5x5 system
   //  Note: See Nishikawa, JCP2014v273pp287-309 for details, which is available at
   //        http://www.hiroakinishikawa.com/My_papers/nishikawa_jcp2014v273pp287-309_preprint.pdf.

   if (trim(grad_type) == "quadratic2") {

   //  Perform Step 1 as below (before actually compute the gradient).

      //do i = 1, nnodes
      for (size_t i = 0; i < E2Ddata.nnodes; i++) {
         //nghbr0 : do k = 1, node[i].nnghbrs;
         for (size_t k = 0; k < E2Ddata.node[i].nnghbrs; k++) {
            in      = (*E2Ddata.node[i].nghbr)(k);
            (*E2Ddata.node[i].dx)(k)      = E2Ddata.node[in].x       - E2Ddata.node[i].x;
            (*E2Ddata.node[i].dy)(k)      = E2Ddata.node[in].y       - E2Ddata.node[i].y;
            (*E2Ddata.node[i].dw)(ivar,k) = (*E2Ddata.node[in].w)(ivar) - (*E2Ddata.node[i].w)(ivar);
         } //end loop nghbr0 nnghbrs
      }//end loop nnodes

   }
   //-------------------------------------------------

   //------------------------------------------------------------
   //------------------------------------------------------------
   //-- Compute LSQ Gradients at all nodes.
   cout << "Compute LSQ Gradients at all nodes. " << endl;
   //------------------------------------------------------------
   //------------------------------------------------------------

   //nodes : loop nnodes
   for (size_t i = 0; i < E2Ddata.nnodes; i++) {

      //-------------------------------------------------
      // Linear LSQ 2x2 system
      if (trim(grad_type) == "linear") {

         lsq_gradients_nc(E2Ddata, i, ivar);
      }
      //-------------------------------------------------
      // Two-step quadratic LSQ 5x5 system
      //  Note: See Nishikawa, JCP2014v273pp287-309 for details, which is available at
      //        http://www.hiroakinishikawa.com/My_papers/nishikawa_jcp2014v273pp287-309_preprint.pdf.
      else if (trim(grad_type) == "quadratic2") {

         //cout << "(trim(grad_type) == quadratic2) " << endl;
         lsq_gradients2_nc(E2Ddata, i, ivar);
      }
      //-------------------------------------------------
      else {

         cout << " Invalid input value -> " << trim(grad_type);
         std::exit(0); //stop

      }
      //-------------------------------------------------

   } //end loop nodes

} // end compute_gradient_nc





//********************************************************************************
//* Compute the gradient, (wx,wy), for the variable u by Linear LSQ.
//*
//* ------------------------------------------------------------------------------
//*  Input:            inode = Node number at which the gradient is computed.
//*                     ivar =   Variable for which the gradient is computed.
//*          node[:).w(ivar) = Solution at nearby nodes.
//*
//* Output:  node[inode].gradu = gradient of the requested variable
//* ------------------------------------------------------------------------------
//*
//********************************************************************************
void EulerSolver2D::Solver::lsq_gradients_nc(
   EulerSolver2D::MainData2D& E2Ddata, int inode, int ivar) {

   //  use edu2d_my_main_data           , only : node
   //  use edu2d_constants              , only : p2, zero

   //  implicit none

   //  integer, intent(in) :: inode, ivar

   //Local variables
   int in, inghbr;
   int ix, iy;
   real da, ax, ay;

   ix = 0;
   iy = 1;
   ax = zero;
   ay = zero;

//   Loop over neighbors

   // loop in = 1, E2Ddata.node[inode].nnghbrs
   for (size_t in = 0; in < E2Ddata.node[inode].nnghbrs; in ++) {
      inghbr = (*E2Ddata.node[inode].nghbr)(in);

      da = (*E2Ddata.node[inghbr].w)(ivar) - (*E2Ddata.node[inode].w)(ivar);

      ax = ax + (*E2Ddata.node[inode].lsq2x2_cx)(in)*da;
      ay = ay + (*E2Ddata.node[inode].lsq2x2_cy)(in)*da;

      // cout << " (*E2Ddata.node[inode].lsq2x2_cx)(in) = "
      //          << (*E2Ddata.node[inode].lsq2x2_cx)(in) << endl;
      // cout << " (*E2Ddata.node[inode].lsq2x2_cy)(in) = "
      //          << (*E2Ddata.node[inode].lsq2x2_cy)(in) << endl;

   }

   (*E2Ddata.node[inode].gradw)(ivar,ix) = ax;  //<-- du(ivar)/dx
   (*E2Ddata.node[inode].gradw)(ivar,iy) = ay;  //<-- du(ivar)/dy

}// end lsq_gradients_nc
//--------------------------------------------------------------------------------



//********************************************************************************
//* Compute the gradient, (wx,wy), for the variable u by Quadratic LSQ.
//*
//* ------------------------------------------------------------------------------
//*  Input:            inode = Node number at which the gradient is computed.
//*                     ivar =   Variable for which the gradient is computed.
//*          node[:).w(ivar) = Solution at nearby nodes.
//*
//* Output:  node[inode].gradu = gradient of the requested variable
//* ------------------------------------------------------------------------------
//*
//********************************************************************************
void EulerSolver2D::Solver::lsq_gradients2_nc(
   EulerSolver2D::MainData2D& E2Ddata, int inode, int ivar) {

   //Local variables
   int  in;
   int  ix, iy, ii, ell, k;
   real da, ax, ay;

   ix = 0;
   iy = 1;
   ax = zero;
   ay = zero;

//   Loop over neighbors

   ii = -1;

   //nghbr : loop node[inode].nnghbrs
   for (size_t k = 0; k < E2Ddata.node[inode].nnghbrs; k++) {
      //in = node[inode].nghbr(k)
      in = (*E2Ddata.node[inode].nghbr)(k);

      //nghbr_nghbr : do ell = 1, node[in].nnghbrs
      for (size_t ell = 0; ell < E2Ddata.node[in].nnghbrs; ell++ ) {

         da = (*E2Ddata.node[in].w)(ivar) - (*E2Ddata.node[inode].w)(ivar) +\
                                          (*E2Ddata.node[in].dw)(ivar,ell);

         if ( (*E2Ddata.node[in].nghbr)(ell) == inode ) {
            da = (*E2Ddata.node[in].w)(ivar) - (*E2Ddata.node[inode].w)(ivar);
         }

         ii = ii + 1;

         ax = ax + (*E2Ddata.node[inode].lsq5x5_cx)(ii) * da;
         ay = ay + (*E2Ddata.node[inode].lsq5x5_cy)(ii) * da;

         // if (inode<E2Ddata.maxit-1) {
         //    cout << "(*E2Ddata.node[inode].lsq5x5_cx)(ii) = " << (*E2Ddata.node[inode].lsq5x5_cx)(ii) << endl;
         //    cout << "(*E2Ddata.node[inode].lsq5x5_cy)(ii) = " << (*E2Ddata.node[inode].lsq5x5_cy)(ii) << endl;
         // }
         if (inode<E2Ddata.maxit-0) {
            cout << " = " << ax << endl;
            cout << " = " << ay << endl;
         }

      } // end loop nghbr_nghbr

   } // end loop nghbr

   (*E2Ddata.node[inode].gradw)(ivar,ix) = ax;  //<-- dw(ivar)/dx;
   (*E2Ddata.node[inode].gradw)(ivar,iy) = ay;  //<-- dw(ivar)/dy;

} //end lsq_gradients2_nc
//--------------------------------------------------------------------------------






//********************************************************************************
//* --- LSQ Coefficients for 2x2 Linear Least-Squares Gradient Reconstruction ---
//*
//* ------------------------------------------------------------------------------
//*  Input:  inode = node number at which the gradient is computed.
//*
//* Output:  node[inode].lsq2x2_cx(:)
//*          node[inode].lsq2x2_cx(:)
//* ------------------------------------------------------------------------------
//*
//********************************************************************************
void EulerSolver2D::Solver::lsq01_2x2_coeff_nc(
   EulerSolver2D::MainData2D& E2Ddata , int inode) {


//  use edu2d_my_main_data           , only : node
//  use edu2d_constants              , only : p2, zero


//  int, intent(in) :: inode

//Local variables
//  real a(2,2), dx, dy, det, w2, w2dvar
//  int  :: k, inghbr, ix=1,iy=2
//  real(p2), dimension(2,2) :: local_lsq_inverse
   Array2D<real> a(2,2), local_lsq_inverse(2,2);
   real dx, dy, det, w2, w2dvar;
   int k, inghbr, ix,iy;
   ix = 0;
   iy = 1;

   a = zero;

//  Loop over the neighbor nodes:  node[inode].nnghbrs
   for (size_t k = 0; k < E2Ddata.node[inode].nnghbrs; k++) {
      inghbr = (*E2Ddata.node[inode].nghbr)(k);

      if (inghbr == inode) {
         cout << "ERROR: nodes must differ//" << endl;
         std::exit(0);
      }
      dx = E2Ddata.node[inghbr].x - E2Ddata.node[inode].x;
      dy = E2Ddata.node[inghbr].y - E2Ddata.node[inode].y;

      w2 = lsq_weight(E2Ddata, dx, dy);
      w2 = w2 * w2;

      a(0,0) = a(0,0) + w2 * dx*dx;
      a(0,1) = a(0,1) + w2 * dx*dy;

      a(1,0) = a(1,0) + w2 * dx*dy;
      a(1,1) = a(1,1) + w2 * dy*dy;


   }//end do

   det = a(0,0)*a(1,1) - a(0,1)*a(1,0);
   if (std::abs(det) < 1.0e-14) {
      cout << " Singular: LSQ det = " <<  det <<  " i= " <<  inode;
      std::exit(0);
    }

   // invert andE2Ddatainverse(0,1) = -a(1,0)/det;
   // local_lsq_inverse(1,0) = -a(0,1)/det;
   // local_lsq_inverse(1,1) =  a(0,0)/det;

   local_lsq_inverse(0,0) =  a(1,1)/det;
   local_lsq_inverse(0,1) = -a(1,0)/det;
   local_lsq_inverse(1,0) = -a(0,1)/det;
   local_lsq_inverse(1,1) =  a(0,0)/det;



   //  Now compute the coefficients for neighbors.

   //nghbr : loop node[inode].nnghbrs
   for (size_t k = 0; k < E2Ddata.node[inode].nnghbrs; k++) {
      inghbr = (*E2Ddata.node[inode].nghbr)(k);

      dx = E2Ddata.node[inghbr].x - E2Ddata.node[inode].x;
      dy = E2Ddata.node[inghbr].y - E2Ddata.node[inode].y;

      w2dvar = lsq_weight(E2Ddata, dx, dy);
      w2dvar = w2dvar * w2dvar;

      (*E2Ddata.node[inode].lsq2x2_cx)(k)  = \
                                   local_lsq_inverse(ix,0)*w2dvar*dx \
                                 + local_lsq_inverse(ix,1)*w2dvar*dy;

      (*E2Ddata.node[inode].lsq2x2_cy)(k)  = \
                                   local_lsq_inverse(iy,0)*w2dvar*dx \
                                 + local_lsq_inverse(iy,1)*w2dvar*dy;

   } //end nghbr loop

}//lsq01_2x2_coeff_nc
//********************************************************************************
//*
//********************************************************************************



//********************************************************************************
//* --- LSQ Coefficients for 5x5 Quadratic Least-Squares Gradient Reconstruction ---
//*
//* Note: See Nishikawa, JCP2014v273pp287-309 for details, which is available at
//*
//* http://www.hiroakinishikawa.com/My_papers/nishikawa_jcp2014v273pp287-309_preprint.pdf.
//*
//* ------------------------------------------------------------------------------
//*  Input:
//*
//* Output:  node[:).lsq5x5_cx(ii)
//*          node[:).lsq5x5_cx(ii)
//*
//* Note: This subroutine computes the LSQ coefficeints at all nodes.
//* ------------------------------------------------------------------------------
//*
//********************************************************************************
void EulerSolver2D::Solver::lsq02_5x5_coeff2_nc(
   EulerSolver2D::MainData2D& E2Ddata ) {

//  use edu2d_my_main_data           , only : node, nnodes
//  use edu2d_constants              , only : p2, zero, half

//  implicit none

// //Local variables
//  real a(5,5), ainv(5,5), dx, dy
//  real dummy1(5), dummy2(5)
//  real w2
//  int  :: istat, ix=1, iy=2
   Array2D<real> a(5,5), ainv(5,5), dummy1(5,1), dummy2(5,1);
   real dx, dy, det, w2;
//  int i, k, ell, in, ii
   int istat;
   int ell, in, ii;
   int ix = 0;
   int iy = 1;

   cout << "     lsq02_5x5_coeff2_nc " << endl;
   cout << "gradient_weight  = " << trim(E2Ddata.gradient_weight) << endl;
   
   
   // Step 1

   //   node1 : loop nnodes
   //    nghbr : loop node[i].nnghbrs
   for (size_t i = 0; i < E2Ddata.nnodes; i++) {
      for (size_t k = 0; k < E2Ddata.node[i].nnghbrs; k++) {
         in      = (*E2Ddata.node[i].nghbr)(k);
         // if (in >= E2Ddata.node[i].nnghbrs) {
         //    cout << "ERROR: in >= nnghbrs " << in << " " << E2Ddata.node[i].nnghbrs << endl;
         //    std::exit(0);
         // }
         //cout << " i = " << i << " k = " << k << endl;
         (*E2Ddata.node[i].dx)(k)   = E2Ddata.node[in].x - E2Ddata.node[i].x;
         (*E2Ddata.node[i].dy)(k)   = E2Ddata.node[in].y - E2Ddata.node[i].y;


      }//    end loop nghbr
   }//   end loop node1


   // Step 2

   //   node2 : loop nnodes
   for (size_t i = 0; i < E2Ddata.nnodes; i++) {

     a = zero;

      //  Get dx, dy, and dw

      //nghbr2 loop k ,E2Ddata.node[i].nnghbrs
      for (size_t k = 0; k < E2Ddata.node[i].nnghbrs; k++) {

         in = (*E2Ddata.node[i].nghbr)(k);

         //nghbr_nghbr : do ell = 1, E2Ddata.node[in].nnghbrs
         for (size_t ell = 0; ell < E2Ddata.node[in].nnghbrs; ell++) {

            dx = E2Ddata.node[in].x - E2Ddata.node[i].x + (*E2Ddata.node[in].dx)(ell);
            dy = E2Ddata.node[in].y - E2Ddata.node[i].y + (*E2Ddata.node[in].dy)(ell);

            if ( (*E2Ddata.node[in].nghbr)(ell) == i ) {
               
               dx = E2Ddata.node[in].x - E2Ddata.node[i].x;
               dy = E2Ddata.node[in].y - E2Ddata.node[i].y;
               if (i < E2Ddata.maxit-1 & ell < E2Ddata.maxit-1) {
                  cout << " --------------E2Ddata.node[in].nghbr)(ell) == i " << endl;
                  cout << " (*E2Ddata.node[in].nghbr)(ell) = " << (*E2Ddata.node[in].nghbr)(ell) << " i = " << i << endl;
                  cout << " dx = " << dx << endl;
                  cout << " dy = " << dy << endl;
                  cout << "  "<< endl;
               }

               if ( std::abs(dx) + std::abs(dy) < 1.0e-13 ) {
                  cout << " Zero distance found at lsq02_5x5_coeff2_nc...\n";
                  cout << "    dx = " << dx << " \n";
                  cout << "    dy = " << dy << " \n";
                  cout << "- Centered node = " << i << "\n";
                  cout << "          (x,y) = " << E2Ddata.node[in].x << E2Ddata.node[in].y << "\n";
                  cout << "- Neighbor node = " << in << "\n";
                  cout << "          (x,y) = " << E2Ddata.node[in].x << E2Ddata.node[in].y << "\n";
                  std::exit(0);
               }

            }

            if (i < E2Ddata.maxit-1 & ell < E2Ddata.maxit-1) {
               cout << " ---------TOP---------- " << endl;
               cout << " E2Ddata.node[in].x = " << E2Ddata.node[in].x << endl;
               cout << " E2Ddata.node[in].y = " << E2Ddata.node[in].y << endl;
               cout << " E2Ddata.node[i].x = " << E2Ddata.node[i].x << endl;
               cout << " E2Ddata.node[i].y = " << E2Ddata.node[i].y << endl;
               cout << " dx = " << dx << endl;
               cout << " dy = " << dy << endl;
               cout << "  "<< endl;
            }

            w2 = lsq_weight(E2Ddata, dx, dy);
            w2 = w2*w2;


            if (i < E2Ddata.maxit-1) {
               cout << " ---------MIDDLE---------- " << endl;
               cout << " dx = " << dx << endl;
               cout << " dy = " << dy << endl;
               cout << " w2 = " << w2 << endl;
               //cout << " half = " << half << endl;
               cout << "  "<< endl;
            }
            a(0,0) = a(0,0) + w2 * dx         *dx;
            a(0,1) = a(0,1) + w2 * dx         *dy;
            a(0,2) = a(0,2) + w2 * dx         *dx*dx * half;
            a(0,3) = a(0,3) + w2 * dx         *dx*dy;
            a(0,4) = a(0,4) + w2 * dx         *dy*dy * half;

            //     a(1,0) = a(1,0) + w2 * dy         *dx;
            a(1,1) = a(1,1) + w2 * dy         *dy;
            a(1,2) = a(1,2) + w2 * dy         *dx*dx * half;
            a(1,3) = a(1,3) + w2 * dy         *dx*dy;
            a(1,4) = a(1,4) + w2 * dy         *dy*dy * half;

            //     a(2,0) = a(2,0) + w2 * half*dx*dx *dx;
            //     a(2,1) = a(2,1) + w2 * half*dx*dx *dy;
            a(2,2) = a(2,2) + w2 * half*dx*dx *dx*dx * half;
            a(2,3) = a(2,3) + w2 * half*dx*dx *dx*dy;
            a(2,4) = a(2,4) + w2 * half*dx*dx *dy*dy * half;

            //     a(3,0) = a(3,0) + w2 *      dx*dy *dx;
            //     a(3,1) = a(3,1) + w2 *      dx*dy *dy;
            //     a(3,2) = a(3,2) + w2 *      dx*dy *dx*dx * half;
            a(3,3) = a(3,3) + w2 *      dx*dy *dx*dy;
            a(3,4) = a(3,4) + w2 *      dx*dy *dy*dy * half;

            //     a(4,0) = a(4,0) + w2 * half*dy*dy *dx;
            //     a(4,1) = a(4,1) + w2 * half*dy*dy *dy;
            //     a(4,2) = a(4,2) + w2 * half*dy*dy *dx*dx * half;
            //     a(4,3) = a(4,3) + w2 * half*dy*dy *dx*dy;
            a(4,4) = a(4,4) + w2 * half*dy*dy *dy*dy * half;

         } //end do nghbr_nghbr

      } //end do nghbr2

      //   Fill symmetric part

      a(1,0) = a(0,1);
      a(2,0) = a(0,2);  a(2,1) = a(1,2);
      a(3,0) = a(0,3);  a(3,1) = a(1,3); a(3,2) = a(2,3);
      a(4,0) = a(0,4);  a(4,1) = a(1,4); a(4,2) = a(2,4); a(4,3) = a(3,4);

      //   Invert the matrix

      if (i < E2Ddata.maxit-1) {
         cout << " a = " << endl;
         print(a);
      }

      dummy1 = zero;
      dummy2 = zero;
      //gewp_solve(a,dummy1,dummy2,ainv,istat, 5);
      //ainv = 0.;// 
      //cout << " invert i = " << i << endl;
      Array2D ca = a;
      MatrixXd me(5,5),ime(5,5);
      for (size_t ie = 0; ie < 5; ie++) {
         for (size_t je = 0; je < 5; je++) {
            me(ie,je) = a(ie,je);
         }
      }
      ime = me.inverse();
      //ainv = a.invert(); // destructive solve //GSinv(a,dummy1,dummy2);
      //ainv = a.inverse();
      for (size_t ie = 0; ie < 5; ie++) {
         for (size_t je = 0; je < 5; je++) {
            ainv(ie,je) = ime(ie,je);
         }
      }

      if (i < E2Ddata.maxit-1) {
         std::ostringstream out;
         out << " ---------BOTTOM---------- " << endl;
         out << " dx = " << dx << endl;
         out << " dy = " << dy << endl;
         out << " w2 = " << w2 << endl;
         E2Ddata.write_diagnostic(out, "log/out.dat");
      }

      if (i < E2Ddata.maxit-1) {
         std::ostringstream out;
         out << " a = \n" << endl;
         a.print(out);
         out << "\n ainv = \n" << endl;
         ainv.print(out);
         //ainv.print();
         E2Ddata.write_diagnostic(out, "log/out.dat");
      }
      if (i < E2Ddata.maxit-1) {
         std::ostringstream out;
         cout << " check 1" << endl;
         Array2D cka1 = matmul(ainv, ca);
         cout << "ainv * a = " << endl;
         print (cka1);

         Array2D cka = matmul(ainv, ca);
         cout << " check 2" << endl;
         Array2D cka2 = matmul(ca, ainv);
         cout << "a * ainv = " << endl;
         print (cka2);
      }
      // if (ainv.istat>=0) {
      //    cout << "Problem in solving the linear system//: Quadratic_LSJ_Matrix \n";
      //    std::exit(0);
      // }

      //  Now compute the coefficients for neighbors.

      ii = -1;

      //nghbr3 : do k = 1, E2Ddata.node[i].nnghbrs
      for (size_t k = 0; k < E2Ddata.node[i].nnghbrs; k++) {
         in = (*E2Ddata.node[i].nghbr)(k);

         //nghbr_nghbr2 : do ell = 1, E2Ddata.node[in].nnghbrs
         for (size_t ell = 0; ell < E2Ddata.node[in].nnghbrs; ell++) {

            dx = E2Ddata.node[in].x - E2Ddata.node[i].x + (*E2Ddata.node[in].dx)(ell);
            dy = E2Ddata.node[in].y - E2Ddata.node[i].y + (*E2Ddata.node[in].dy)(ell);

            if ( (*E2Ddata.node[in].nghbr)(ell) == i )  {
               dx = E2Ddata.node[in].x - E2Ddata.node[i].x;
               dy = E2Ddata.node[in].y - E2Ddata.node[i].y;
            }

            ii = ii + 1;

            w2 = lsq_weight(E2Ddata, dx, dy);
            w2 = w2 * w2;

            

 //  Multiply the inverse LSQ matrix to get the coefficients: cx(:) and cy(:):

            (*E2Ddata.node[i].lsq5x5_cx)(ii)  =   ainv(ix,0)*w2*dx  \
                                                + ainv(ix,1)*w2*dy             \
                                                + ainv(ix,2)*w2*dx*dx * half   \
                                                + ainv(ix,3)*w2*dx*dy          \
                                                + ainv(ix,4)*w2*dy*dy * half;

            (*E2Ddata.node[i].lsq5x5_cy)(ii)  =   ainv(iy,0)*w2*dx  \
                                                + ainv(iy,1)*w2*dy             \
                                                + ainv(iy,2)*w2*dx*dx * half   \
                                                + ainv(iy,3)*w2*dx*dy          \
                                                + ainv(iy,4)*w2*dy*dy * half;
         }//end do nghbr_nghbr2

      }//end do nghbr3

   }//   end do node2

} //end  lsq02_5x5_coeff2_nc
//********************************************************************************
//*
//********************************************************************************


//****************************************************************************
//* Compute the LSQ weight
//*
//* Note: The weight computed here is the square of the actual LSQ weight.
//*****************************************************************************
real EulerSolver2D::Solver::lsq_weight(
   EulerSolver2D::MainData2D& E2Ddata, real dx, real dy) {

   //  use edu2d_constants   , only : p2, one
   //  use edu2d_my_main_data, only : gradient_weight, gradient_weight_p

   //Output
   real lsq_weight;

   //Local
   real distance;

   if (trim(E2Ddata.gradient_weight) == "none") {

      lsq_weight = one;
   }
   else if (trim(E2Ddata.gradient_weight) == "inverse_distance") {

      // cout << "INVERSE DISTANCE  = " << endl;
      // std::exit(0);


      distance = std::sqrt(dx*dx + dy*dy);

      real val = std::pow(distance, E2Ddata.gradient_weight_p);
      if (val < 1.e-6) {
         cout << "ERROR: distance is 0" << endl;
         std::exit(0);
      }

      lsq_weight = one / val;
   }
   
   //cout << " lsq_weight = " << lsq_weight << endl;

   return lsq_weight;
} //end lsq_weight






//****************************************************************************
//* ---------------------------- GAUSS Seidel -------------------------------
//*
//*  This computes the inverse of an (Nm)x(Nm) matrix "ai".
//*
//*****************************************************************************
Array2D<real> EulerSolver2D::Solver::GSinv(const Array2D<real>& a,
                                    Array2D<real>& m, const Array2D<real>& b) {
   return GaussSeidelInv(a,m,b);
}
