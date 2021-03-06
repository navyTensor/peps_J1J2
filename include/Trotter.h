#ifndef TROTTER_H
#define TROTTER_H

#include <iostream>
#include <fstream>
#include <vector>

#include <btas/common/blas_cxx_interface.h>
#include <btas/common/TVector.h>
#include <btas/DENSE/TArray.h>

using std::ostream;
using std::vector;

using namespace btas;

/**
 * @author Brecht Verstichel
 * @date 13-05-2014\n\n
 * This class Trotter contains the two-site gates for the imaginary time evolution in the trotter decomposition
 */
class Trotter {

   public:

      Trotter();

      Trotter(double tau);

      Trotter(const Trotter &);

      virtual ~Trotter();

      double gtau() const;

      const DArray<3> &gLO_n() const;
      const DArray<3> &gRO_n() const;
      
      const DArray<3> &gLO_nn() const;
      const DArray<3> &gRO_nn() const;

   private:
      
      //!Nearest-neigbour Trotter Operators: Left and Right
      DArray<3> LO_n;
      DArray<3> RO_n;

      //!Next-nearest-neigbour Trotter Operators: Left and Right
      DArray<3> LO_nn;
      DArray<3> RO_nn;

      //!timestep
      double tau;


};

#endif

/* vim: set ts=3 sw=3 expandtab :*/
