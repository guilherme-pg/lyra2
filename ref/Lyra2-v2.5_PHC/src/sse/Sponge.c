/**
 * A simple implementation of Blake2b's and BlaMka's internal permutation 
 * in the form of a sponge. SSE-optimized implementation.
 * 
 * Author: The Lyra PHC team (http://www.lyra2.net/) -- 2014.
 * 
 * This software is hereby placed in the public domain.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <string.h>
#include <stdio.h>
#include <immintrin.h>
#include "blake2b-round.h"
#include "Sponge.h"
#include "Lyra2.h"

/**
 * Execute G function, with all 12 rounds.
 * 
 * @param v     A 1024-bit (8 __m128i) array to be processed by Blake2b's or BlaMka's G function
 */
static inline void spongeLyra(__m128i *v){
    __m128i t0, t1;

#if (SPONGE == 0)
    ROUND(0);
    ROUND(1);
    ROUND(2);
    ROUND(3);
    ROUND(4);
    ROUND(5);
    ROUND(6);
    ROUND(7);
    ROUND(8);
    ROUND(9);
    ROUND(10);
    ROUND(11);
#endif
    
#if (SPONGE == 1)
    ROUND_LYRA_BLAMKA(0);
    ROUND_LYRA_BLAMKA(1);
    ROUND_LYRA_BLAMKA(2);
    ROUND_LYRA_BLAMKA(3);
    ROUND_LYRA_BLAMKA(4);
    ROUND_LYRA_BLAMKA(5);
    ROUND_LYRA_BLAMKA(6);
    ROUND_LYRA_BLAMKA(7);
    ROUND_LYRA_BLAMKA(8);
    ROUND_LYRA_BLAMKA(9);
    ROUND_LYRA_BLAMKA(10);
    ROUND_LYRA_BLAMKA(11);
    ROUND_LYRA_BLAMKA(12);
    ROUND_LYRA_BLAMKA(13);
    ROUND_LYRA_BLAMKA(14);
    ROUND_LYRA_BLAMKA(15);
    ROUND_LYRA_BLAMKA(16);
    ROUND_LYRA_BLAMKA(17);
    ROUND_LYRA_BLAMKA(18);
    ROUND_LYRA_BLAMKA(19);
    ROUND_LYRA_BLAMKA(20);
    ROUND_LYRA_BLAMKA(21);
    ROUND_LYRA_BLAMKA(22);
    ROUND_LYRA_BLAMKA(23);
#endif
}

/**
 * Executes a reduced version of G function with only one round
 * @param v     A 1024-bit (8 __m128i) array to be processed by Blake2b's or BlaMka's G function
 */
static inline void reducedSpongeLyra(__m128i *v){
    __m128i t0, t1;

#if (SPONGE == 0)
    ROUND(0);
#endif
    
#if (SPONGE == 1)
    ROUND_LYRA_BLAMKA(0);
#endif
}

/**
 * Initializes the Sponge State. The first 512 bits are set to zeros and the remainder 
 * receive Blake2b's IV as per Blake2b's specification. <b>Note:</b> Even though sponges
 * typically have their internal state initialized with zeros, Blake2b's G function
 * has a fixed point: if the internal state and message are both filled with zeros. the 
 * resulting permutation will always be a block filled with zeros; this happens because 
 * Blake2b does not use the constants originally employed in Blake2 inside its G function, 
 * relying on the IV for avoiding possible fixed points.
 * 
 * @param state         The 1024-bit array to be initialized
 */
inline void initState(__m128i state[/*8*/]){
    //first 512 bits are zeros
    memset(state, 0, 64); 
    //Remainder BLOCK_LEN_BLAKE2_SAFE_BYTES are reserved to the IV
    state[4] = _mm_load_si128((__m128i *) &blake2b_IV[0]);
    state[5] = _mm_load_si128((__m128i *) &blake2b_IV[2]);
    state[6] = _mm_load_si128((__m128i *) &blake2b_IV[4]);
    state[7] = _mm_load_si128((__m128i *) &blake2b_IV[6]);
}


/**
 * Performs an absorb operation for a single block (BLOCK_LEN_BLAKE2_SAFE_INT64 
 * words of type __m128i), using G function as the internal permutation
 * 
 * @param state The current state of the sponge 
 * @param in    The block to be absorbed (BLOCK_LEN_BLAKE2_SAFE_INT64 words)
 */
inline void absorbBlockBlake2Safe(__m128i *state, const __m128i *in) {
    //XORs the first BLOCK_LEN_BLAKE2_SAFE_INT64 words of "in" with the current state
    state[0] = _mm_xor_si128(state[0], in[0]);
    state[1] = _mm_xor_si128(state[1], in[1]);
    state[2] = _mm_xor_si128(state[2], in[2]);
    state[3] = _mm_xor_si128(state[3], in[3]);

    //Applies the transformation f to the sponge's state
    spongeLyra(state);
}

/** 
 * Performs a reduced squeeze operation for a single row, from the highest to 
 * the lowest index, using the reduced-round G function as the 
 * internal permutation
 * 
 * @param state     The current state of the sponge 
 * @param rowOut    Row to receive the data squeezed
 */
inline void reducedSqueezeRow0(__m128i* state, __m128i* rowOut) {
    __m128i* ptrWord = rowOut + (N_COLS-1)*BLOCK_LEN_INT128; //In Lyra2: pointer to M[0][C-1]
    int i, j;
    //M[row][C-1-col] = H.reduced_squeeze()    
    for (i = 0; i < N_COLS; i++) {
        for (j = 0; j < BLOCK_LEN_INT128; j++){
            ptrWord[j] = state[j];
        }

        //Goes to next block (column) that will receive the squeezed data
        ptrWord -= BLOCK_LEN_INT128;

        //Applies the reduced-round transformation f to the sponge's state
        reducedSpongeLyra(state);
    }
}


/** 
 * Performs a reduced duplex operation for a single row, from the highest to 
 * the lowest index, using the reduced-round G function as the 
 * internal permutation
 * 
 * @param state		The current state of the sponge 
 * @param rowIn		Row to feed the sponge
 * @param rowOut	Row to receive the sponge's output
 */
inline void reducedDuplexRow1(__m128i *state, __m128i *rowIn, __m128i *rowOut) {
    __m128i* ptrWordIn = rowIn;                                 //In Lyra2: pointer to prev
    __m128i* ptrWordOut = rowOut + (N_COLS-1)*BLOCK_LEN_INT128; //In Lyra2: pointer to row
    int i, j;

    for (i = 0; i < N_COLS; i++) {

	//Absorbing "M[prev][col]"
        for (j = 0; j < BLOCK_LEN_INT128; j++){
            state[j] = _mm_xor_si128(state[j], ptrWordIn[j]);
        }

	//Applies the reduced-round transformation f to the sponge's state
        reducedSpongeLyra(state);

	//M[row][C-1-col] = M[prev][col] XOR rand
        for (j = 0; j < BLOCK_LEN_INT128; j++){
            ptrWordOut[j] = _mm_xor_si128(state[j], ptrWordIn[j]);
        }

	//Input: next column (i.e., next block in sequence)
	ptrWordIn += BLOCK_LEN_INT128;
	//Output: goes to previous column
	ptrWordOut -= BLOCK_LEN_INT128;
    }
}



/**
 * Performs a duplexing operation over "M[rowInOut][col] [+] M[rowIn][col]",
 * where [+] denotes wordwise addition, ignoring carries between words, for all
 * values of col in the [0,N_COLS[ interval. The output of this operation, 
 * "rand", is then used to make "M[rowOut][(N_COLS-1)-col] = M[rowIn][col] XOR rand" 
 * and "M[rowInOut][col] =  M[rowInOut][col] XOR rotL(rand)", where rotL is 
 * a 128-bit rotation to the left and N_COLS is a system parameter.
 *
 * @param state          The current state of the sponge 
 * @param rowInOut       Row used as input and to receive output after rotation
 * @param rowIn          Row used only as input
 * @param rowOut         Row receiving the output
 *
 */
inline void reducedDuplexRow2(__m128i *state, __m128i *rowInOut, __m128i *rowIn, __m128i *rowOut) {
    __m128i* ptrWordIn = rowIn;                                 //In Lyra2: pointer to prev0, the last row ever initialized (=M[1])
    __m128i* ptrWordInOut = rowInOut;				//In Lyra2: pointer to row1, to be revisited and updated (=M[0])
    __m128i* ptrWordOut = rowOut + (N_COLS-1)*BLOCK_LEN_INT128; //In Lyra2: pointer to row0, to be initialized (=M[2])
    
    int i, j;

    for (i = 0; i < N_COLS; i++) {
	//Absorbing "M[0] [+] M[1]"
        for (j = 0; j < BLOCK_LEN_INT128; j++){
            state[j] = _mm_xor_si128(state[j] , _mm_add_epi64(ptrWordInOut[j], ptrWordIn[j]));
        }

	//Applies the reduced-round sponge transformation to its state
        reducedSpongeLyra(state);

        //"M[2][(N_COLS-1)-col] = M[1][col] XOR rand" 
        for (j = 0; j < BLOCK_LEN_INT128; j++){
            ptrWordOut[j] = _mm_xor_si128(state[j], ptrWordIn[j]);
        }
        
        //"M[0][col] =  M[0][col] XOR rotL(rand)"
        for (j = 0; j < BLOCK_LEN_INT128; j++){
            ptrWordInOut[j] = _mm_xor_si128(ptrWordInOut[j], state[(j+1) % BLOCK_LEN_INT128]);
        }
        
	//Inputs: next column (i.e., next block in sequence)
	ptrWordInOut += BLOCK_LEN_INT128;
	ptrWordIn += BLOCK_LEN_INT128;
	//Output: goes to previous column
	ptrWordOut -= BLOCK_LEN_INT128;
    }
}

/**
 * Performs a duplexing operation over 
 * "M[rowInOut][col] [+] M[rowIn0][col] [+] M[rowIn1][col]", where [+] denotes 
 * wordwise addition, ignoring carries between words, , for all values of "col" 
 * in the [0,N_COLS[ interval. The  output of this operation, "rand", is then 
 * employed to make  
 * "M[rowOut][(N_COLS-1)-col] = M[rowIn0][col] XOR rand" and 
 * "M[rowInOut][col] =  M[rowInOut][col] XOR rotL(rand)", 
 * where rotL is a 128-bit rotation to the left and N_COLS is a system parameter.
 *
 * @param state          The current state of the sponge 
 * @param rowInOut       Row used as input and to receive output after rotation
 * @param rowIn0         Row used only as input
 * @param rowIn1         Another row used only as input
 * @param rowOut         Row receiving the output
 *
 */
inline void reducedDuplexRowFilling(__m128i *state, __m128i *rowInOut, __m128i *rowIn0, __m128i *rowIn1, __m128i *rowOut) {
    __m128i* ptrWordIn0 = rowIn0;				//In Lyra2: pointer to prev0, the last row ever initialized
    __m128i* ptrWordIn1 = rowIn1;				//In Lyra2: pointer to prev1, the last row ever revisited and updated
    __m128i* ptrWordInOut = rowInOut;				//In Lyra2: pointer to row1, to be revisited and updated
    __m128i* ptrWordOut = rowOut + (N_COLS-1)*BLOCK_LEN_INT128; //In Lyra2: pointer to row0, to be initialized
    
    int i, j; 
    
    __m128i stateLocal[8];
    for (i=0; i< 8; i++) {stateLocal[i] = state[i];}
    
    for (i = 0; i < N_COLS; i++) { 
	//Absorbing "M[row1] [+] M[prev0] [+] M[prev1]"
        for (j = 0; j < BLOCK_LEN_INT128; j++){
            stateLocal[j] = _mm_xor_si128(stateLocal[j], _mm_add_epi64(_mm_add_epi64(ptrWordInOut[j],ptrWordIn0[j]),ptrWordIn1[j]));
        }
        
	//Applies the reduced-round transformation f to the sponge's state
        reducedSpongeLyra(stateLocal);
        
	//M[row0][col] = M[prev0][col] XOR rand        
        for (j = 0; j < BLOCK_LEN_INT128; j++){
            ptrWordOut[j] = _mm_xor_si128(stateLocal[j], ptrWordIn0[j]);
        }
        
	//M[row1][col] = M[row1][col] XOR rotRt(rand)        
        for (j = 0; j < BLOCK_LEN_INT128; j++){
            ptrWordInOut[j] = _mm_xor_si128(ptrWordInOut[j], stateLocal[(j+1) % BLOCK_LEN_INT128]);
        }
	//Inputs: next column (i.e., next block in sequence)
	ptrWordInOut += BLOCK_LEN_INT128;
	ptrWordIn0 += BLOCK_LEN_INT128;
	ptrWordIn1 += BLOCK_LEN_INT128;
	//Output: goes to previous column
	ptrWordOut -= BLOCK_LEN_INT128;
    }
    
    state[0] = stateLocal[0];
    state[1] = stateLocal[1];
    state[2] = stateLocal[2];
    state[3] = stateLocal[3];
    state[4] = stateLocal[4];
    state[5] = stateLocal[5];
    state[6] = stateLocal[6];
    state[7] = stateLocal[7];
}

/**
 * Performs a duplexing operation over 
 * "M[rowInOut0][col] [+] M[rowInOut1][col] [+] M[rowIn0][col_0] [+] M[rowIn1][col_1]", 
 * where [+] denotes wordwise addition, ignoring carries between words. The value of
 * "col_0" is computed as "MSW(rand) mod N_COLS", and "col_1" as 
 * "MSB(rotL(rand)) mod N_COLS", where MSW means "the most significant word" 
 * (assuming 128-bit words), rotL is a 128-bit  rotation to the left, 
 * N_COLS is a system parameter, and "rand" corresponds
 * to the sponge's output for each column absorbed.
 * The same output is then employed to make 
 * "M[rowInOut0][col] = M[rowInOut0][col] XOR rand" and 
 * "M[rowInOut1][col] = M[rowInOut1][col] XOR rotL(rand)".
 *
 * @param rowIn          Row used only as input
 * @param rowInOut       Row used as input and to receive output after rotation
 * @param rowOut         Row receiving the output
 * 
 * @param state          The current state of the sponge 
 * @param rowInOut0      Row used as input and to receive output
 * @param rowInOut1      Row used as input and to receive output after rotation
 * @param rowIn0         Row used only as input
 * @param rowIn1         Another row used only as input
 *
 */
inline unsigned int reducedDuplexRowWandering(__m128i *state, __m128i *rowInOut0, __m128i *rowInOut1, __m128i *rowIn0, __m128i *rowIn1) {
    __m128i* ptrWordInOut0 = rowInOut0; //In Lyra2: pointer to row0
    __m128i* ptrWordInOut1 = rowInOut1; //In Lyra2: pointer to row1
    __m128i* ptrWordIn0;                //In Lyra2: pointer to prev0
    __m128i* ptrWordIn1;                //In Lyra2: pointer to prev1
    uint64_t randomColumn;
    unsigned int col = 0;
    
    int i, j;
    
    __m128i stateLocal[8];
    for (i=0; i< 8; i++) {stateLocal[i] = state[i];}
    
    for (i = 0; i < N_COLS; i++) { 
        
        //col_0 = LSW(rotRt^2(rand)) mod N_COLS
        //randomColumn = (((uint64_t)((__uint128_t *)stateLocal)[2]) & (N_COLS-1))*BLOCK_LEN_INT128;          /*(USE THIS IF N_COLS IS A POWER OF 2)*/
        randomColumn =  (((uint64_t)((__uint128_t *)stateLocal)[2]) % N_COLS)*BLOCK_LEN_INT128;               /*(USE THIS FOR THE "GENERIC" CASE)*/
        col = randomColumn / BLOCK_LEN_INT128;
        ptrWordIn0 = rowIn0 + randomColumn; 
        
        //col_1 = LSW(rotRt^3(rand)) mod N_COLS
        //randomColumn = (((uint64_t)((__uint128_t *)stateLocal)[3]) & (N_COLS-1))*BLOCK_LEN_INT128;          /*(USE THIS IF N_COLS IS A POWER OF 2)*/
        randomColumn =  (((uint64_t)((__uint128_t *)stateLocal)[3]) % N_COLS)*BLOCK_LEN_INT128;               /*(USE THIS FOR THE "GENERIC" CASE)*/
        ptrWordIn1 = rowIn1 + randomColumn; 
        
    	//Absorbing "M[row0] [+] M[row1] [+] M[prev0] [+] M[prev1]"
        for (j = 0; j < BLOCK_LEN_INT128; j++){
            stateLocal[j] = _mm_xor_si128(stateLocal[j], _mm_add_epi64( _mm_add_epi64( ptrWordInOut0[j] , ptrWordInOut1[j] ) , _mm_add_epi64( ptrWordIn0[j] , ptrWordIn1[j] )));
        }

	//Applies the reduced-round transformation f to the sponge's state
        reducedSpongeLyra(stateLocal);

	//M[rowInOut0][col] = M[rowInOut0][col] XOR rand
        for (j = 0; j < BLOCK_LEN_INT128; j++){
            ptrWordInOut0[j] = _mm_xor_si128(stateLocal[j], ptrWordInOut0[j]);
        }        
        
        //M[rowInOut1][col] = M[rowInOut1][col] XOR rotRt(rand)
        for (j = 0; j < BLOCK_LEN_INT128; j++){
            ptrWordInOut1[j] = _mm_xor_si128(ptrWordInOut1[j], stateLocal[(j+1) % BLOCK_LEN_INT128]);
        }

	//Goes to next block
        ptrWordInOut0 += BLOCK_LEN_INT128;
        ptrWordInOut1 += BLOCK_LEN_INT128; 
    }
    
    state[0] = stateLocal[0];
    state[1] = stateLocal[1];
    state[2] = stateLocal[2];
    state[3] = stateLocal[3];
    state[4] = stateLocal[4];
    state[5] = stateLocal[5];
    state[6] = stateLocal[6];
    state[7] = stateLocal[7];
    return col;
}

/**
 * Performs a duplexing operation over 
 * "M[rowInOut0][col] [+] M[rowInP][col] [+] M[rowIn0][col_0]", 
 * where [+] denotes wordwise addition, ignoring carries between words. The value of
 * "col_0" is computed as "MSW(rand) mod N_COLS",where MSW means "the most significant word" 
 * (assuming 128-bit words), rotL is a 128-bit  rotation to the left, 
 * N_COLS is a system parameter, and "rand" corresponds
 * to the sponge's output for each column absorbed.
 * The same output is then employed to make 
 * "M[rowInOut0][col] = M[rowInOut0][col] XOR rotL(rand)".
 *
 * @param rowIn          Row used only as input
 * @param rowInOut       Row used as input and to receive output after rotation
 * @param rowOut         Row receiving the output
 * 
 * @param state          The current state of the sponge 
 * @param rowInOut0      Row used as input and to receive output after rotation
 * @param rowIn1         Row used only as input (row from another thread)
 * @param rowIn0         Another row used only as input
 *
 */
inline void reducedDuplexRowWanderingParallel(__m128i *state, __m128i *rowInOut0, __m128i *rowInP, __m128i *rowIn0) {
    __m128i* ptrWordInOut0 = rowInOut0;                 //In Lyra2: pointer to row0
    __m128i* ptrWordInP = rowInP;                       //In Lyra2: pointer to row0_p
    __m128i* ptrWordIn0;                                //In Lyra2: pointer to prev0
    uint64_t randomColumn;

    int i, j;
    
    __m128i stateLocal[8];
    for (i=0; i< 8; i++) {stateLocal[i] = state[i];}
    
    for (i = 0; i < N_COLS; i++) {
        //col0 = LSW(rotRt^3(rand)) mod N_COLS
        //randomColumn = (((uint64_t)((__uint128_t *)stateLocal)[3]) & (N_COLS-1))*BLOCK_LEN_INT128;            /*(USE THIS IF N_COLS IS A POWER OF 2)*/
        randomColumn = (((uint64_t)((__uint128_t *)stateLocal)[3]) % N_COLS)*BLOCK_LEN_INT128;                  /*(USE THIS FOR THE "GENERIC" CASE)*/
        ptrWordIn0 = rowIn0 + randomColumn; 
        
        //Absorbing "M[row0] [+] M[prev0] [+] M[row0p]"
        for (j = 0; j < BLOCK_LEN_INT128; j++) {
            stateLocal[j] = _mm_xor_si128(stateLocal[j], _mm_add_epi64( ptrWordInOut0[j] , _mm_add_epi64( ptrWordIn0[j] , ptrWordInP[j] )));
        } 
        
        //Applies the reduced-round transformation f to the sponge's state
        reducedSpongeLyra(stateLocal);
        
        //M[rowInOut0][col] = M[rowInOut0][col] XOR rand
        for (j = 0; j < BLOCK_LEN_INT128; j++){
            ptrWordInOut0[j] = _mm_xor_si128(ptrWordInOut0[j], stateLocal[j]);
        }
        
        //Goes to next block
        ptrWordInOut0 += BLOCK_LEN_INT128;
        ptrWordInP += BLOCK_LEN_INT128; 
    }
    
    state[0] = stateLocal[0];
    state[1] = stateLocal[1];
    state[2] = stateLocal[2];
    state[3] = stateLocal[3];
    state[4] = stateLocal[4];
    state[5] = stateLocal[5];
    state[6] = stateLocal[6];
    state[7] = stateLocal[7];
}

/**
 * Performs an absorb operation of single column from "in", the 
 * said column being pseudorandomly picked in the range [0, BLOCK_LEN_INT64[, 
 * using the full-round G function as the internal permutation
 * 
 * @param state The current state of the sponge 
 * @param in    The row whose column (BLOCK_LEN_INT128 words) should be absorbed 
 */
inline void absorbRandomColumn(__m128i *state, __m128i *in, unsigned int col) {
    //picks a pseudorandom column from "in"
    //__m128i* ptrWordIn = in + (((uint64_t)((__uint128_t *)state)[5]) & (N_COLS-1))*BLOCK_LEN_INT128;        /*(USE THIS IF N_COLS IS A POWER OF 2)*/
    __m128i* ptrWordIn = in + col*BLOCK_LEN_INT128;              /*(USE THIS FOR THE "GENERIC" CASE)*/
    
    int i;
    
    //absorbs the column picked
    for (i = 0; i < BLOCK_LEN_INT128; i++){
        state[i] = _mm_xor_si128( state[i], ptrWordIn[i]);
    }

    //Applies the full-round transformation f to the sponge's state
    spongeLyra(state);
}

/**
 * Performs a squeeze operation, using G function as the 
 * internal permutation
 * 
 * @param state      The current state of the sponge 
 * @param out        Array that will receive the data squeezed
 * @param len        The number of bytes to be squeezed into the "out" array
 */
void squeeze(__m128i *state, byte *out, unsigned int len) {
    int fullBlocks = len / BLOCK_LEN_BYTES;
    byte *ptr = out;
    int i;
    //Squeezes full blocks
    for (i = 0; i < fullBlocks; i++) {
        memcpy(ptr, state, BLOCK_LEN_BYTES);
        spongeLyra(state);

        ptr += BLOCK_LEN_BYTES;
    }
    memcpy(ptr, state, (len % BLOCK_LEN_BYTES));
}

/**
 Prints an array of unsigned chars
 */
void printArray(unsigned char *array, unsigned int size, char *name) {
    int i;
    printf("%s: ", name);
    for (i = 0; i < size; i++) {
        printf("%2x|", array[i]);
    }
    printf("\n");
}
////////////////////////////////////////////////////////////////////////////////////////////////
