/**
 * @file This file is part of LoadBalance.
 *
 * @section LICENSE
 * MIT License
 *
 * Copyright (c) 2018 Rajdeep Konwar
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @section DESCRIPTION
 * Balance computation load across MPI ranks
 **/

#include <iostream>
#include <mpi.h>
#include <cmath>
#include <vector>

#define MASTER 0

template< typename T >
T randomizer( const T &i_low,
              const T &i_high ) {
  return (static_cast< T > (rand()) /
          static_cast< T > (RAND_MAX / (i_high - i_low)) + i_low);
}

int main( int    i_argc,
          char **i_argv ) {
  //! Initialize MPI
  MPI_Init( &i_argc, &i_argv );

  //! Number of ranks
  int l_worldSize;
  MPI_Comm_size( MPI_COMM_WORLD, &l_worldSize );

  //! Current rank
  int l_worldRank;
  MPI_Comm_rank( MPI_COMM_WORLD, &l_worldRank );

  int                   l_noAngles, l_balancedSize;
  std::vector< double > l_masterVec, l_slaveVec, l_tmpVec;

  if( l_worldRank == MASTER ) {
    std::cout << "Number of ranks = " << l_worldSize << std::endl << std::endl;

    //! Obtain angles from slaves (1 to N-1)
    for( int l_i = 1; l_i < l_worldSize; l_i++ ) {
      //! Receive number of angles to help resize temp vector
      MPI_Recv( &l_noAngles, 1, MPI_INT, l_i, 0, MPI_COMM_WORLD,
                MPI_STATUS_IGNORE );
      std::cout << "Received no. of angles = " << l_noAngles << " from rank "
                << l_i << std::endl;

      //! Resize temp vector and receive the slave-angles
      l_tmpVec.resize( l_noAngles );
      MPI_Recv( &l_tmpVec[0], l_noAngles, MPI_DOUBLE, l_i, 0, MPI_COMM_WORLD,
                MPI_STATUS_IGNORE );

      std::cout << "Received vector by master: ";
      for( size_t l_j = 0; l_j < l_tmpVec.size(); l_j++ ) {
        std::cout << l_tmpVec[l_j] << " ";

        //! Populate master vector containing all angles obtained from slaves
        l_masterVec.push_back( l_tmpVec[l_j] );
      }
      std::cout << std::endl << std::endl;
    }

    std::cout << "Master vector (" << l_masterVec.size() << "): ";
    l_balancedSize  = l_masterVec.size() / (l_worldSize - 1);

    for( size_t l_i = 0; l_i < l_masterVec.size(); l_i++ )
      std::cout << l_masterVec[l_i] << " ";
    std::cout << std::endl << std::endl;

    int l_count = 0;

    //! Sending balanced vectors to slaves
    for( int l_i = 1; l_i < l_worldSize; l_i++ ) {
      l_tmpVec.clear();

      for( int l_j = 0; l_j < l_balancedSize; l_j++ )
        l_tmpVec.push_back( l_masterVec[l_count++] );

      //! Push back any remaining angles in last rank
      while( (l_count != l_masterVec.size()) && (l_i == l_worldSize - 1) )
        l_tmpVec.push_back( l_masterVec[l_count++] );

      int l_tmpSize = (int) l_tmpVec.size();
      //! Send number of balanced angles to slaves
      MPI_Send( &l_tmpSize, 1, MPI_INT, l_i, 0, MPI_COMM_WORLD );

      //! Send balanced vector to slaves
      MPI_Send( &l_tmpVec[0], l_tmpSize, MPI_DOUBLE, l_i, 0, MPI_COMM_WORLD );
    }

    //! Prep master vector to store sin values
    l_masterVec.clear();

    //! Receive sin vector from slaves
    for( int l_i = 1; l_i < l_worldSize; l_i++ ) {
      //! Receive number of angles to help resize temp vector
      MPI_Recv( &l_noAngles, 1, MPI_INT, l_i, 0, MPI_COMM_WORLD,
                MPI_STATUS_IGNORE );

      //! Resize temp vector and receive the slave-angles
      l_tmpVec.resize( l_noAngles );
      MPI_Recv( &l_tmpVec[0], l_noAngles, MPI_DOUBLE, l_i, 0, MPI_COMM_WORLD,
                MPI_STATUS_IGNORE );

      //! Populate master vector with sin values obtained from slaves
      for( size_t l_j = 0; l_j < l_tmpVec.size(); l_j++ )
        l_masterVec.push_back( l_tmpVec[l_j] );
    }

    std::cout << "Final Master vector (" << l_masterVec.size() << "): ";
    for( size_t l_i = 0; l_i < l_masterVec.size(); l_i++ )
      std::cout << l_masterVec[l_i] << " ";
    std::cout << std::endl;
  } else {
    //! Seed randomizer
    srand( time( nullptr ) + l_worldRank );

    //! Randomize number of angles in slave rank
    l_noAngles = randomizer< int >( 1, 50 );

    //! Send number of angles to master
    MPI_Send( &l_noAngles, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD );

    //! Push into an empty vector
    l_slaveVec.clear();
    for( int l_i = 0; l_i < l_noAngles; l_i++ )
      l_slaveVec.push_back( randomizer< double >( 0.0, 360.0 ) );

    //! Send vector to master
    MPI_Send( &l_slaveVec[0], l_noAngles, MPI_DOUBLE, MASTER, 0, MPI_COMM_WORLD );

    //! Receive number of angles in balanced vector
    MPI_Recv( &l_balancedSize, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD,
              MPI_STATUS_IGNORE );

    //! Resize slave vector and receive balanced vector of angles
    l_slaveVec.resize( l_balancedSize );
    MPI_Recv( &l_slaveVec[0], l_balancedSize, MPI_DOUBLE, MASTER, 0,
              MPI_COMM_WORLD, MPI_STATUS_IGNORE );

    std::cout << "Received vector by slave " << l_worldRank << " ("
              << l_slaveVec.size() << "): ";
    for( size_t l_j = 0; l_j < l_slaveVec.size(); l_j++ ) {
      std::cout << l_slaveVec[l_j] << "->";

      //! Calculate sin(x) in-place
      l_slaveVec[l_j] = sin( l_slaveVec[l_j] );

      std::cout << "(" << l_slaveVec[l_j] << ") ";
    }
    std::cout << std::endl << std::endl;

    //! Send number of angles in sin vector
    MPI_Send( &l_balancedSize, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD );

    //! Send sin vector back to master
    MPI_Send( &l_slaveVec[0], l_balancedSize, MPI_DOUBLE, MASTER, 0,
              MPI_COMM_WORLD );
  }

  //! Finalize MPI
  MPI_Finalize();

  return EXIT_SUCCESS;
}