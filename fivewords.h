#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

#define MAXWORDS 15000
#define MAXNEIGHBOURS 10000

// Structure to capture words that can coexist with a chosen word
typedef struct NeighbourStruct
{
	uint32_t Neighbour[MAXNEIGHBOURS];	// List of all other words (by index) that could be used with this word
	uint32_t NumNeighbours;			// How many neighbours does this word have
} NeighbourList;

// Structure for an entry in our dictionary
typedef struct WordStruct
{
	char Characters[64]; 			// The actual word - 5 letters + (optional)\r + \n + \0
	uint32_t LetterMap;			// Bitfield map of which letters are used (for fast comparison)
	NeighbourList Neighbours;		// List of all other words (by index) that could be used with this word
} Word;

uint32_t Sequences = 0;				// How many sets of 5 words have we found?
Word *Dictionary;				// Our word list
uint32_t NumWords = 0;				// How many words in the dictionary
pthread_mutex_t CalcMutex;			// Thread synchronization for calculations
pthread_mutex_t OutMutex;			// Thread synchronization for final output
uint32_t NextWord = 0;				// Which starting word are we trying next?
static volatile uint8_t Running = 0;		// Have we finished precompute and started the search?

