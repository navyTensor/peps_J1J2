#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <complex>
#include <omp.h>

using std::cout;
using std::endl;
using std::vector;
using std::complex;
using std::ofstream;

#include "include.h"

using namespace global;

/** 
 * empty constructor
 */
Environment::Environment(){ }

/** 
 * constructor with allocation
 * @param D_in bond dimension of peps state
 * @param D_aux_in contraction bond dimension
 * @param comp_sweeps_in sets the number of sweeps done for MPO compression
 */
Environment::Environment(int D_in,int D_aux_in,int comp_sweeps_in){

   t.resize(Ly - 2);
   b.resize(Ly - 2);

   D = D_in;
   D_aux = D_aux_in;
   comp_sweeps = comp_sweeps_in;

   //allocate the memory
   
   //bottom
   int tmp = D*D;

   for(int i = 0;i < Ly - 2;++i){

      if(tmp < D_aux){

         b[i] = MPO<double>(Lx,D,tmp);
         tmp *= D*D;

      }
      else
         b[i] = MPO<double>(Lx,D,D_aux);

   }
   
   //top
   tmp = D*D;

   for(int i = Ly - 3;i >= 0;--i){

      if(tmp < D_aux){

         t[i] = MPO<double>(Lx,D,tmp);
         tmp *= D*D;

      }
      else
         t[i] = MPO<double>(Lx,D,D_aux);

   }

}

/** 
 * copy constructor with allocation
 */
Environment::Environment(const Environment &env_copy){

   t = env_copy.gt();
   b = env_copy.gb();

   D = env_copy.gD();
   D_aux = env_copy.gD_aux();

   comp_sweeps = env_copy.gcomp_sweeps();

}

/**
 * empty destructor
 */
Environment::~Environment(){ }

/**
 * construct the enviroment mps's for the input PEPS
 * @param option if 'L' construct full left environment
 *               if 'R' construct full right environment
 *               if 'T' construct full top environment
 *               if 'B' construct full bottom environment
 *               if 'A' construct all environments
 * @param peps input PEPS<double>
 * @param D_aux dimension to which environment will be compressed
 */
void Environment::calc(const char option,PEPS<double> &peps){

   if(option == 'A'){

      b[0].fill('b',peps);
      b[0].canonicalize(Right,false);

      for(int i = 1;i < Ly - 2;++i)
         this->add_layer('b',i,peps);

      t[Ly - 3].fill('t',peps);
      t[Ly - 3].canonicalize(Right,false);

      for(int i = Ly - 4;i >= 0;--i)
         this->add_layer('t',i,peps);

   }
   else if(option == 'B'){

      b[0].fill('b',peps);
      b[0].canonicalize(Right,false);

      for(int i = 1;i < Ly - 2;++i)
         this->add_layer('b',i,peps);

   }
   else if(option == 'T'){

      t[Ly - 3].fill('t',peps);
      t[Ly - 3].canonicalize(Right,false);

      for(int i = Ly - 4;i >= 0;--i)
         this->add_layer('t',i,peps);

   }

}

/**
 * test if the enviroment is correctly contracted
 */
void Environment::test(){

   for(int i = 0;i < Ly - 3;++i)
      cout << i + 2 << "\t" << b[i + 1].dot(t[i]) << endl;

}

/**
 * const version
 * @param row the row index
 * @return the top boundary 'MPO' environment on row 'row'
 */
const MPO<double> &Environment::gt(int row) const {

   return t[row];

}

/**
 * access version
 * @param row the row index
 * @return the top boundary 'MPO' environment on row 'row'
 */
MPO<double> &Environment::gt(int row) {

   return t[row];

}

/**
 * const version
 * @param row the row index
 * @return the bottom boundary 'MPO' environment on row 'row'
 */
const MPO<double> &Environment::gb(int row) const {

   return b[row];

}

/**
 * access version
 * @param row the row index
 * @return the bottom boundary 'MPO' environment on row 'row'
 */
MPO<double> &Environment::gb(int row) {

   return b[row];

}

/**
 * @return the auxiliary bond dimension for the contraction
 **/
int Environment::gD_aux() const {

   return D_aux;

}

/**
 * @return the auxiliary bond dimension for the contraction
 **/
int Environment::gD() const {

   return D;

}

/**
 * @return the number of sweeps performed during compression
 **/
int Environment::gcomp_sweeps() const {

   return comp_sweeps;

}


/**
 * set a new bond dimension
 */
void Environment::sD(int D_in) {

   D = D_in;

}

/**
 * set a new auxiliary bond dimension
 */
void Environment::sD_aux(int D_aux_in) {

   D_aux = D_aux_in;

}

/**
 * @return the full bottom boundary 'MPO'
 */
const vector< MPO<double> > &Environment::gb() const {

   return b;

}

/**
 * @return the full top boundary 'MPO'
 */
const vector< MPO<double> > &Environment::gt() const {

   return t;

}

/**
 * construct the (t or b) environment on row/col 'rc' by adding a the appropriate peps row/col and compressing the boundary MPO
 * @param option 't'op or 'b'ottom
 * @param row row index
 * @param peps the input PEPS<double> object 
 */
void Environment::add_layer(const char option,int row,PEPS<double> &peps){

   //initialize using svd: output is right normalized b/t[row]
   init_svd(option,row,peps);

   if(option == 'b'){

#ifdef _DEBUG
      cout << endl;
      cout << "compression of bottom row\t" << row << endl;
      cout << endl;
#endif

      std::vector< DArray<4> > R(Lx+1);

      //first construct rightmost operator
      R[Lx].resize(1,1,1,1);
      R[Lx] = 1.0;

      //now move from right to left to construct the rest
      for(int col = Lx - 1;col > 0;--col){

         DArray<6> tmp6;
         Contract(1.0,b[row - 1][col],shape(3),R[col+1],shape(0),0.0,tmp6);

         DArray<7> tmp7;
         Contract(1.0,tmp6,shape(1,3),peps(row,col),shape(3,4),0.0,tmp7);

         tmp6.clear();
         Contract(1.0,tmp7,shape(1,2,6),peps(row,col),shape(3,4,2),0.0,tmp6);

         Contract(1.0,tmp6,shape(3,5,1),b[row][col],shape(1,2,3),0.0,R[col]);

      }

      R[0].resize(1,1,1,1);
      R[0] = 1.0;

      int iter = 0;

      while(iter < comp_sweeps){

#ifdef _DEBUG
         cout << iter  << "\t" << cost_function('b',row,0,peps,R) << endl;
#endif

         //now for the rest of the rightgoing sweep.
         for(int i = 0;i < Lx-1;++i){

            DArray<6> tmp6;
            Contract(1.0,R[i],shape(0),b[row - 1][i],shape(0),0.0,tmp6);

            DArray<7> tmp7;
            Contract(1.0,tmp6,shape(0,3),peps(row,i),shape(0,3),0.0,tmp7);

            tmp6.clear();
            Contract(1.0,tmp7,shape(0,2,5),peps(row,i),shape(0,3,2),0.0,tmp6);

            Contract(1.0,tmp6,shape(1,3,5),R[i+1],shape(0,1,2),0.0,b[row][i]);

            //QR
            DArray<2> tmp2;
            Geqrf(b[row][i],tmp2);

            //add tmp2 to next b
            DArray<4> tmp4;
            Contract(1.0,tmp2,shape(1),b[row][i+1],shape(0),0.0,tmp4);

            b[row][i+1] = std::move(tmp4);

            //construct new left 'R' matrix

            Contract(1.0,tmp6,shape(0,2,4),b[row][i],shape(0,1,2),0.0,R[i+1]);

         }

         //back to the beginning with a leftgoing sweep
         for(int i = Lx-1;i > 0;--i){

            DArray<6> tmp6;
            Contract(1.0,b[row - 1][i],shape(3),R[i+1],shape(0),0.0,tmp6);

            DArray<7> tmp7;
            Contract(1.0,peps(row,i),shape(3,4),tmp6,shape(1,3),0.0,tmp7);

            tmp6.clear();
            Contract(1.0,peps(row,i),shape(2,3,4),tmp7,shape(2,4,5),0.0,tmp6);

            DArray<6> tmp6bis;
            Permute(tmp6,shape(4,2,0,3,1,5),tmp6bis);

            Gemm(CblasTrans,CblasNoTrans,1.0,R[i],tmp6bis,0.0,b[row][i]);

            //LQ
            DArray<2> tmp2;
            Gelqf(tmp2,b[row][i]);

            //construct next right operator
            Gemm(CblasNoTrans,CblasTrans,1.0,tmp6bis,b[row][i],0.0,R[i]);

            //multiply the tmp2 with the next tensor:
            DArray<4> tmp4;
            Gemm(CblasNoTrans,CblasNoTrans,1.0,b[row][i-1],tmp2,0.0,tmp4);

            b[row][i-1] = std::move(tmp4);

         }

         ++iter;

      }

      //redistribute the norm over the chain
      double nrm =  Nrm2(b[row][0]);

      //rescale the first site
      Scal((1.0/nrm), b[row][0]);

      //then multiply the norm over the whole chain
      b[row].scal(nrm);

   }
   else{

#ifdef _DEBUG
      cout << endl;
      cout << "compression of top row\t"  << row << endl;
      cout << endl;
#endif

      //peps index is row+2!
      int prow = row+2;

      std::vector< DArray<4> > R(Lx+1);

      //first construct rightmost operator
      R[Lx].resize(1,1,1,1);
      R[Lx] = 1.0;

      //now move from right to left to construct the rest
      for(int col = Lx - 1;col > 0;--col){

         DArray<6> tmp6;
         Contract(1.0,t[row + 1][col],shape(3),R[col+1],shape(0),0.0,tmp6);

         DArray<7> tmp7;
         Contract(1.0,tmp6,shape(1,3),peps(prow,col),shape(1,4),0.0,tmp7);

         tmp6.clear();
         Contract(1.0,tmp7,shape(1,5,2),peps(prow,col),shape(1,2,4),0.0,tmp6);

         Contract(1.0,tmp6,shape(3,5,1),t[row][col],shape(1,2,3),0.0,R[col]);

      }

      R[0].resize(1,1,1,1);
      R[0] = 1.0;

      int iter = 0;

      while(iter < comp_sweeps){

#ifdef _DEBUG
         cout << iter  << "\t" << cost_function('t',row,0,peps,R) << endl;
#endif

         //now for the rest of the rightgoing sweep.
         for(int i = 0;i < Lx-1;++i){

            DArray<6> tmp6;
            Contract(1.0,R[i],shape(0),t[row + 1][i],shape(0),0.0,tmp6);

            DArray<7> tmp7;
            Contract(1.0,tmp6,shape(0,3),peps(prow,i),shape(0,1),0.0,tmp7);

            tmp6.clear();
            Contract(1.0,tmp7,shape(0,2,4),peps(prow,i),shape(0,1,2),0.0,tmp6);

            Contract(1.0,tmp6,shape(1,3,5),R[i+1],shape(0,1,2),0.0,t[row][i]);

            //QR
            DArray<2> tmp2;
            Geqrf(t[row][i],tmp2);

            //add tmp2 to next t
            DArray<4> tmp4;
            Contract(1.0,tmp2,shape(1),t[row][i+1],shape(0),0.0,tmp4);

            t[row][i+1] = std::move(tmp4);

            //construct new left 'R' matrix
            Contract(1.0,tmp6,shape(0,2,4),t[row][i],shape(0,1,2),0.0,R[i+1]);

         }

         //back to the beginning with a leftgoing sweep
         for(int i = Lx-1;i > 0;--i){

            DArray<6> tmp6;
            Contract(1.0,t[row + 1][i],shape(3),R[i+1],shape(0),0.0,tmp6);

            DArray<7> tmp7;
            Contract(1.0,peps(prow,i),shape(1,4),tmp6,shape(1,3),0.0,tmp7);

            tmp6.clear();
            Contract(1.0,peps(prow,i),shape(1,2,4),tmp7,shape(4,1,5),0.0,tmp6);

            DArray<6> tmp6bis;
            Permute(tmp6,shape(4,2,0,3,1,5),tmp6bis);

            Gemm(CblasTrans,CblasNoTrans,1.0,R[i],tmp6bis,0.0,t[row][i]);

            //LQ
            DArray<2> tmp2;
            Gelqf(tmp2,t[row][i]);

            //construct next right operator
            Gemm(CblasNoTrans,CblasTrans,1.0,tmp6bis,t[row][i],0.0,R[i]);

            //multiply the tmp2 with the next tensor:
            DArray<4> tmp4;
            Gemm(CblasNoTrans,CblasNoTrans,1.0,t[row][i-1],tmp2,0.0,tmp4);

            t[row][i-1] = std::move(tmp4);

         }

         ++iter;

      }

      //redistribute the norm over the chain
      double nrm =  Nrm2(t[row][0]);

      //rescale the first site
      Scal((1.0/nrm), t[row][0]);

      //then multiply the norm over the whole chain
      t[row].scal(nrm);

   }

}

/**
 * construct the (t or b) environment on row/col 'rc' by adding a the appropriate peps row/col and compressing the boundary MPO
 * @param option 't'op or 'b'ottom
 * @param row row index
 * @param col column index
 * @param peps the input PEPS<double> object 
 */
double Environment::cost_function(const char option,int row,int col,const PEPS<double> &peps,const std::vector< DArray<4> > &R){

   if(option == 'b'){

      //environment of b is completely unitary
      double val = Dot(b[row][col],b[row][col]);

      //add row -1 to right hand side (col + 1)
      DArray<6> tmp6;
      Contract(1.0,b[row - 1][col],shape(3),R[col+1],shape(0),0.0,tmp6);

      DArray<7> tmp7;
      Contract(1.0,tmp6,shape(1,3),peps(row,col),shape(3,4),0.0,tmp7);

      tmp6.clear();
      Contract(1.0,tmp7,shape(6,1,2),peps(row,col),shape(2,3,4),0.0,tmp6);

      DArray<4> tmp4;
      Contract(1.0,tmp6,shape(3,5,1),b[row][col],shape(1,2,3),0.0,tmp4);

      val -= 2.0 * Dot(tmp4,R[col]);

      return val;

   }
   else{

      int prow = row + 2;

      //environment of b is completely unitary
      double val = Dot(t[row][col],t[row][col]);

      //add row + 1 to right hand side (col + 1)
      DArray<6> tmp6;
      Contract(1.0,t[row + 1][col],shape(3),R[col+1],shape(0),0.0,tmp6);

      DArray<7> tmp7;
      Contract(1.0,tmp6,shape(1,3),peps(prow,col),shape(1,4),0.0,tmp7);

      tmp6.clear();
      Contract(1.0,tmp7,shape(1,5,2),peps(prow,col),shape(1,2,4),0.0,tmp6);

      DArray<4> tmp4;
      Contract(1.0,tmp6,shape(3,5,1),t[row][col],shape(1,2,3),0.0,tmp4);

      val -= 2.0 * Dot(tmp4,R[col]);

      return val;

   }

}

/**
 * initialize the environment on 'row' by performing an svd-compression on the 'full' environment b[row-1] * peps(row,...) * peps(row,...)
 * output is right canonical, which is needed for the compression algorithm!
 * @param option 'b'ottom or 't'op environment
 * @param row index of the row to be added into the environment
 */
void Environment::init_svd(char option,int row,const PEPS<double> &peps){

   if(option == 'b'){

      //first (leftmost) site
      DArray<5> tmp5;
      Contract(1.0,peps(row,0),shape(0,3),b[row-1][0],shape(0,2),0.0,tmp5);

      DArray<6> tmp6;
      Contract(1.0,peps(row,0),shape(2,3),tmp5,shape(1,3),0.0,tmp6);

      DArray<6> tmp6bis;
      Permute(tmp6,shape(0,1,3,2,4,5),tmp6bis);

      //now svd the large object
      DArray<1> S;
      DArray<4> VT;

      Gesvd('S','S',tmp6bis,S,b[row][0],VT,D_aux);

      //paste S to VT for next iteration
      Dimm(S,VT);

      for(int col = 1;col < Lx - 1;++col){

         //add next bottom to 'VT' from previous column
         tmp6.clear();
         Contract(1.0,VT,shape(3),b[row-1][col],shape(0),0.0,tmp6);

         //add next peps(row,col) to intermediate
         DArray<7> tmp7;
         Contract(1.0,tmp6,shape(1,3),peps(row,col),shape(0,3),0.0,tmp7);

         //and again
         tmp6.clear();
         Contract(1.0,tmp7,shape(1,5,2),peps(row,col),shape(0,2,3),0.0,tmp6);

         //permute!
         tmp6bis.clear();
         Permute(tmp6,shape(0,2,4,3,5,1),tmp6bis);

         //and svd!
         S.clear();
         VT.clear();

         Gesvd('S','S',tmp6bis,S,b[row][col],VT,D_aux);

         //paste S to VT for next iteration
         Dimm(S,VT);

      }

      //last site: Lx - 1

      //add next bottom to 'VT' from previous column
      tmp6.clear();
      Contract(1.0,VT,shape(3),b[row-1][Lx-1],shape(0),0.0,tmp6);

      //add next peps(row,col) to intermediate
      DArray<7> tmp7;
      Contract(1.0,tmp6,shape(1,3),peps(row,Lx-1),shape(0,3),0.0,tmp7);

      //and again
      tmp6.clear();
      Contract(1.0,tmp7,shape(1,5,2),peps(row,Lx-1),shape(0,2,3),0.0,tmp6);

      //permute!
      tmp6bis.clear();
      Permute(tmp6,shape(0,2,4,3,5,1),tmp6bis);

      //move to darray<4>
      DArray<4> tmp4 = tmp6bis.reshape_clear( shape(tmp6bis.shape(0),tmp6bis.shape(1),tmp6bis.shape(2),1) );

      //and svd!
      S.clear();
      DArray<2> R;

      //different svd!
      Gesvd('S','S',tmp4,S,R,b[row][Lx-1],D_aux);

      //paste S to VT for next iteration
      Dimm(R,S);

      //the rest of the columns
      for(int col = Lx - 2;col > 0;--col){

         //first paste R onto the next b
         DArray<4> tmp4bis;
         Contract(1.0,b[row][col],shape(3),R,shape(0),0.0,tmp4bis);

         //svd
         S.clear();
         R.clear();
         Gesvd('S','S',tmp4bis,S,R,b[row][col],D_aux);

         //paste S to VT for next iteration
         Dimm(R,S);

      }

      //finally paste R to the first b
      DArray<4> tmp4bis;
      Contract(1.0,b[row][0],shape(3),R,shape(0),0.0,tmp4bis);

      b[row][0] = std::move(tmp4bis);

   }
   else{

      //peps index is row+2!
      int prow = row+2;

      //first (leftmost) site
      DArray<5> tmp5;
      Contract(1.0,t[row+1][0],shape(0,1),peps(prow,0),shape(0,1),0.0,tmp5);

      DArray<6> tmp6;
      Contract(1.0,tmp5,shape(0,2),peps(prow,0),shape(1,2),0.0,tmp6);

      DArray<6> tmp6bis;
      Permute(tmp6,shape(3,1,4,0,2,5),tmp6bis);

      //now svd the large object
      DArray<1> S;
      DArray<4> VT;

      Gesvd('S','S',tmp6bis,S,t[row][0],VT,D_aux);

      //paste S to VT for next iteration
      Dimm(S,VT);

      for(int col = 1;col < Lx - 1;++col){

         //add next top to 'VT' from previous column
         tmp6.clear();
         Contract(1.0,VT,shape(1),t[row+1][col],shape(0),0.0,tmp6);

         //add next peps(row,col) to intermediate
         DArray<7> tmp7;
         Contract(1.0,tmp6,shape(1,3),peps(prow,col),shape(0,1),0.0,tmp7);

         //and again
         tmp6.clear();
         Contract(1.0,tmp7,shape(1,2,4),peps(prow,col),shape(0,1,2),0.0,tmp6);

         //permute!
         tmp6bis.clear();
         Permute(tmp6,shape(0,2,4,1,3,5),tmp6bis);

         //and svd!
         S.clear();
         VT.clear();

         Gesvd('S','S',tmp6bis,S,t[row][col],VT,D_aux);

         //paste S to VT for next iteration
         Dimm(S,VT);

      }

      //last site: Lx - 1
      tmp6.clear();
      Contract(1.0,VT,shape(1),t[row+1][Lx-1],shape(0),0.0,tmp6);

      //add next peps(row,col) to intermediate
      DArray<7> tmp7;
      Contract(1.0,tmp6,shape(1,3),peps(prow,Lx-1),shape(0,1),0.0,tmp7);

      //and again
      tmp6.clear();
      Contract(1.0,tmp7,shape(1,2,4),peps(prow,Lx-1),shape(0,1,2),0.0,tmp6);

      //permute!
      tmp6bis.clear();
      Permute(tmp6,shape(0,2,4,1,3,5),tmp6bis);

      //move to darray<4>
      DArray<4> tmp4 = tmp6bis.reshape_clear( shape(tmp6bis.shape(0),tmp6bis.shape(1),tmp6bis.shape(2),1) );

      //and svd!
      S.clear();
      DArray<2> R;

      //different svd!
      Gesvd('S','S',tmp4,S,R,t[row][Lx-1],D_aux);

      //paste S to VT for next iteration
      Dimm(R,S);

      //the rest of the columns
      for(int col = Lx - 2;col > 0;--col){

         //first paste R onto the next b
         DArray<4> tmp4bis;
         Contract(1.0,t[row][col],shape(3),R,shape(0),0.0,tmp4bis);

         //svd
         S.clear();
         R.clear();
         Gesvd('S','S',tmp4bis,S,R,t[row][col],D_aux);

         Dimm(R,S);

      }

      //finally paste R to the first b
      DArray<4> tmp4bis;
      Contract(1.0,t[row][0],shape(3),R,shape(0),0.0,tmp4bis);

      t[row][0] = std::move(tmp4bis);

   }

}
