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
T randomizer(const T& low, const T& high)
{
	return (static_cast<T>(rand()) / static_cast<T>(RAND_MAX / (high - low)) + low);
}

int main(int argc, char ** argv)
{
	//! Initialize MPI
	MPI_Init(&argc, &argv);

	//! Number of ranks
	int worldSize;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

	//! Current rank
	int worldRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);

	int numAngles, balancedSize;
	std::vector<double> masterVec, slaveVec, tmpVec;

	if (worldRank == MASTER)
	{
		std::cout << "Number of ranks = " << worldSize << std::endl << std::endl;

		//! Obtain angles from slaves (1 to N-1)
		for (int i = 1; i < worldSize; i++)
		{
			//! Receive number of angles to help resize temp vector
			MPI_Recv(&numAngles, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			std::cout << "Received no. of angles = " << numAngles << " from rank " << i << std::endl;

			//! Resize temp vector and receive the slave-angles
			tmpVec.resize(static_cast<size_t>(numAngles));
			MPI_Recv(&tmpVec[0], numAngles, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			std::cout << "Received vector by master: ";
			for (size_t j = 0; j < tmpVec.size(); j++)
			{
				std::cout << tmpVec[j] << " ";

				//! Populate master vector containing all angles obtained from slaves
				masterVec.push_back(tmpVec[j]);
			}
			std::cout << std::endl << std::endl;
		}

		std::cout << "Master vector (" << masterVec.size() << "): ";
		balancedSize = static_cast<int>(masterVec.size()) / (worldSize - 1);

		for (size_t i = 0; i < masterVec.size(); i++)
			std::cout << masterVec[i] << " ";
		std::cout << std::endl << std::endl;

		int count = 0;

		//! Sending balanced vectors to slaves
		for (int i = 1; i < worldSize; i++)
		{
			tmpVec.clear();

			for (int j = 0; j < balancedSize; j++)
				tmpVec.push_back(masterVec[count++]);

			//! Push back any remaining angles in last rank
			while ((count != masterVec.size()) && (i == worldSize - 1))
				tmpVec.push_back(masterVec[count++]);

			int tmpSize = static_cast<int>(tmpVec.size());
			//! Send number of balanced angles to slaves
			MPI_Send(&tmpSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

			//! Send balanced vector to slaves
			MPI_Send(&tmpVec[0], tmpSize, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
		}

		//! Prep master vector to store sin values
		masterVec.clear();

		//! Receive sin vector from slaves
		for (int i = 1; i < worldSize; i++)
		{
			//! Receive number of angles to help resize temp vector
			MPI_Recv(&numAngles, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			//! Resize temp vector and receive the slave-angles
			tmpVec.resize(static_cast<size_t>(numAngles));
			MPI_Recv(&tmpVec[0], numAngles, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			//! Populate master vector with sin values obtained from slaves
			for (size_t j = 0; j < tmpVec.size(); j++)
				masterVec.push_back(tmpVec[j]);
		}

		std::cout << "Final Master vector (" << masterVec.size() << "): ";
		for (size_t i = 0; i < masterVec.size(); i++)
			std::cout << masterVec[i] << " ";
		std::cout << std::endl;
	}
	else
	{
		//! Seed randomizer
		srand(time(nullptr) + worldRank);

		//! Randomize number of angles in slave rank
		numAngles = randomizer<int>(1, 50);

		//! Send number of angles to master
		MPI_Send(&numAngles, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD);

		//! Push into an empty vector
		slaveVec.clear();
		for (int i = 0; i < numAngles; i++)
			slaveVec.push_back(randomizer<double>(0.0, 360.0));

		//! Send vector to master
		MPI_Send(&slaveVec[0], numAngles, MPI_DOUBLE, MASTER, 0, MPI_COMM_WORLD);

		//! Receive number of angles in balanced vector
		MPI_Recv(&balancedSize, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		//! Resize slave vector and receive balanced vector of angles
		slaveVec.resize(static_cast<size_t>(balancedSize));
		MPI_Recv(&slaveVec[0], balancedSize, MPI_DOUBLE, MASTER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		std::cout << "Received vector by slave " << worldRank << " (" << slaveVec.size() << "): ";
		for (size_t j = 0; j < slaveVec.size(); j++)
		{
			std::cout << slaveVec[j] << "->";

			//! Calculate sin(x) in-place
			slaveVec[j] = sin(slaveVec[j]);

			std::cout << "(" << slaveVec[j] << ") ";
		}
		std::cout << std::endl << std::endl;

		//! Send number of angles in sin vector
		MPI_Send(&balancedSize, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD);

		//! Send sin vector back to master
		MPI_Send(&slaveVec[0], balancedSize, MPI_DOUBLE, MASTER, 0, MPI_COMM_WORLD);
	}

	//! Finalize MPI
	MPI_Finalize();

	return EXIT_SUCCESS;
}
