#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static const int TAG_ARRAY = 0;
static const int TAG_LAST = 1;

int main(int argc, char *argv[])
{   
  int myRank;
  int numProc;

  double startTime = 0.0;
  double endTime = 0.0;
  
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank); // Set each processors rank
  MPI_Comm_size(MPI_COMM_WORLD, &numProc);  // Get the number of processors

  if (argc < 2){
    if (myRank == 0){
      printf("Invalid arguments, use ./main n\n");
    }
      
    MPI_Finalize();
    return 0;
  }
  
  startTime = MPI_Wtime();  // Start timer
  
  long n = atol(argv[1]); // Read n from arguments
  unsigned long nSquared = n * n;  // Calculate n^2

  unsigned long squareHalf = ((nSquared - n) / 2) + n; // Subtract diagonal before halving then re-add diagonal

  unsigned long procWeights[numProc];  // Processor weights array
  for (int i = 0; i < numProc; i++){
    procWeights[i] = floor((float) squareHalf / numProc); // Evenly divide the numbers amongst processors
  
    if (i < squareHalf % numProc){
      procWeights[i] += 1;
    } // If the upper half isn't evenly divisible by number of processors, add 1 to each processor until exausted
  }

  MPI_Barrier(MPI_COMM_WORLD);

  unsigned long endPos = 0;  // Last position in previous processors section
  unsigned long counter = 1;
  
  for (int i = 0; i < myRank; i++){
    endPos += procWeights[i];  // Find end of previous process' section in the 1D array
  }
  /*printf("Processor [%d] endPos = [%lu]\n", myRank, endPos);*/

  unsigned long i = 1;
  unsigned long j = 1; // Start at first number (1x1)

  while (counter <= endPos){
    counter++;
    i++;
    if (i == (n + 1)){	// Can't index outside of the matrix
        j++;
        i = j;
    }
  }
  /*printf("Processor [%d] starting location: [%lu][%lu]\n", myRank, j, i);*/

  unsigned long sect = procWeights[myRank];  // Get section size
  unsigned char *productArray = (unsigned char*) calloc(nSquared + 1, sizeof(unsigned char));  // Initialize empty char array
  /*printf(" Processor [%d] successfully allocated product array\n", myRank);*/
  if (productArray == NULL){
      printf("Failed to allocate product array\n");
  }

  unsigned long product;
  counter = 0;
  while (counter < sect){
    product = i * j;  // Calculate product
    /*printf("Processor [%d] calculating [%llu] x [%llu] = [%llu]\n", myRank, j, i, product);*/
    
    if (product > nSquared) {
		/*printf("WARNING: Processor [%d] calculating [%lu] x [%lu] = [%lu]\n", myRank, j, i, product);*/
		product = nSquared;
    }
    
    productArray[product] = 'a';  // Set product flag to found
    /*printf("Processor [%d] inserted [%lu] : [%lu]\n", myRank, product, productArray[product]);*/
    i++;
    counter++;
    if (i == (n + 1)){	// Can't index outside of the matrix
      j++;
      i = j;
    }
  }

	/*for(counter = 1; counter <= nSquared; counter++){
		printf("Processor [%d] | [%lu][%lu]\n", myRank, counter, productArray[counter]);
	}*/

	unsigned long temp = nSquared + 1;	// Product indices start at 1

	unsigned char* slice;
	i = 0;
	j = temp % 30000;	// Calculate remainder after slicing
	unsigned long k = temp / 30000;	// Split the array into slices of 30000 chars
	/*printf("Processor [%d] rem [%lu] div [%lu]\n", myRank, j, k);*/

  if (myRank != 0){ // Not root process
  	while(k > 0){
  		slice = &productArray[i];
	    MPI_Send(slice, 30000, MPI_UNSIGNED_CHAR, 0, TAG_ARRAY, MPI_COMM_WORLD);
	    i += 30000;
	    k--;
    }
    slice = &productArray[i];
    MPI_Send(slice, j, MPI_UNSIGNED_CHAR, 0, TAG_LAST, MPI_COMM_WORLD);	// Send remainder slice
    free(productArray);
  }

  if (myRank == 0){ // Root process

    unsigned char *otherArray = (unsigned char*) calloc(nSquared + 1, sizeof(unsigned char));
    if (otherArray == NULL){
    	printf("Failed to allocate other array\n");
  	}
    for (int rank = 1; rank < numProc; rank++){

    	i = 0;
    	k = temp / 30000;
    	while(k > 0){

			MPI_Recv(otherArray, 30000, MPI_UNSIGNED_CHAR, rank, TAG_ARRAY, MPI_COMM_WORLD, NULL);
			for (counter = 0; counter < 30000; counter++){
				if(otherArray[counter]){ // No effect if productArray[counter] already == 1
				  productArray[i] = 'a';
				}
				/*printf("Process[%d] received [%lu] i [%lu]\n", myRank, otherArray[counter], counter);*/
				i++;
			}
			k--;
		}
		MPI_Recv(otherArray, j, MPI_UNSIGNED_CHAR, rank, TAG_LAST, MPI_COMM_WORLD, NULL);	// Receive remainder slice
		for (counter = 0; counter < j; counter++){
			/*printf("Process[%d] sent [%lu]\n", rank, otherArray[counter]);*/
			if(otherArray[counter]){ // No effect if productArray[counter] already == 1
			  productArray[i] = 'a';
			}
			/*printf("Process[%d] received [%lu] i [%lu]\n", myRank, otherArray[counter], counter);*/
			i++;
		}
    }

    unsigned long total = 0;
    for (counter = 1; counter <= nSquared; counter++){
      /*printf("Read [%lu] [%lu]\n", counter, productArray[counter]);*/
      if(productArray[counter]){ // Count the total different numbers
        total++;
      }
    }

    free(otherArray);
    free(productArray);

    printf("Total: %lu\n", total);
    
    endTime = MPI_Wtime();
    printf("Time elapsed: %.2lf seconds\n", endTime - startTime);
  }

  MPI_Finalize();
  return 0;
}
