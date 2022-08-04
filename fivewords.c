#include "fivewords.h"

uint32_t Intersect(NeighbourList *NeighboursA, NeighbourList *NeighboursB, NeighbourList *Results)
{
	uint32_t IndexA = 0;
	uint32_t IndexB = 0;
	Results->NumNeighbours = 0;
	
	while(1)
	{
		if ((IndexB >= NeighboursB->NumNeighbours) || (IndexA >= NeighboursA->NumNeighbours))
			return Results->NumNeighbours;

		if (NeighboursA->Neighbour[IndexA] > NeighboursB->Neighbour[IndexB])
			IndexB++;
		else if (NeighboursA->Neighbour[IndexA] < NeighboursB->Neighbour[IndexB])
			IndexA++;
		else
		{
			// Neighbour match!
			Results->Neighbour[Results->NumNeighbours++] = NeighboursA->Neighbour[IndexA];
			IndexA++;
			IndexB++;
		}
	}
}	

void *Worker(void *Arg)
{
	
	// Allocate some local buffers
	NeighbourList WordList3, WordList4, WordList5;
	Word *LocalDictionary = malloc(sizeof(Word) * MAXWORDS);
	if (!LocalDictionary)
	{
		printf("ERROR: Thread cannot allocate local dictionary!\n");
		exit(1);
	}

	// Wait for it...
	while (Running == 0)
	{
	}
	
	while(1)
	{
		// Grab a word ID to evaluate
		pthread_mutex_lock(&CalcMutex);
		int Word1Index = NextWord++;
		pthread_mutex_unlock(&CalcMutex);
		
		if (Word1Index > NumWords)
		{
			return 0;
		}

		Word *Word1Ptr = &Dictionary[Word1Index];
		for (int Word2 = 0; Word2 < Word1Ptr->Neighbours.NumNeighbours; Word2++)
		{
			uint32_t Word2Index = Word1Ptr->Neighbours.Neighbour[Word2];
			Word *Word2Ptr = &Dictionary[Word2Index];

			uint32_t Results3 = Intersect(&Word1Ptr->Neighbours, &Word2Ptr->Neighbours, &WordList3);

			// Recalculate a local dictionary from the words that intersect word1 and word2 (i.e. WordList3)
			// Populate the masks of all words in WordList3
			for (int i = 0; i < Results3; i++)
			{
				uint32_t WordIndex = WordList3.Neighbour[i];
				LocalDictionary[WordIndex].LetterMap = Dictionary[WordIndex].LetterMap;
			}

			// And then compute the neighbour lists for all words.  Keep track of words with sufficient peers that they could be a solution.
			uint32_t Word3Candidates[1000];			
			uint32_t NumWord3Candidates = 0;

			for (int ReWord1 = 0; ReWord1 < Results3; ReWord1++)
			{
				Word *Word1Ptr = &LocalDictionary[WordList3.Neighbour[ReWord1]];
				Word1Ptr->Neighbours.NumNeighbours = 0;
				
				for (int ReWord2 = ReWord1+1; ReWord2 < Results3; ReWord2++)
				{
					int ReWord2Index = WordList3.Neighbour[ReWord2];
					Word *Word2Ptr = &LocalDictionary[ReWord2Index];
					
					if ((Word1Ptr->LetterMap & Word2Ptr->LetterMap) == 0)
					{
						// Word2 shares no letters with word1
						Word1Ptr->Neighbours.Neighbour[Word1Ptr->Neighbours.NumNeighbours++] = ReWord2Index;
						if (Word1Ptr->Neighbours.NumNeighbours == MAXNEIGHBOURS)
						{
							printf("ERROR: Too many neighbours!  Increase MAXNEIGHBOURS in fivewords.h\n");
							return 0;
						}
					}
				}
				if (Word1Ptr->Neighbours.NumNeighbours > 3)
				{
					Word3Candidates[NumWord3Candidates++] = WordList3.Neighbour[ReWord1];
				}

			}	

			// Evaluate possible solutions
			if (NumWord3Candidates)
			{
				for (int Word3 = 0; Word3 < NumWord3Candidates; Word3++)
				{
					uint32_t Word3Index = Word3Candidates[Word3];
					Word *Word3Ptr = &LocalDictionary[Word3Index];
					uint32_t Results4 = Intersect(&WordList3, &Word3Ptr->Neighbours, &WordList4);
					if (Results4 > 1) // Need at least 2 more words!
					{					
						for (int Word4 = 0; Word4 < Results4; Word4++)
						{
							uint32_t Word4Index = WordList4.Neighbour[Word4];
							Word *Word4Ptr = &LocalDictionary[Word4Index];
							uint32_t Results5 = Intersect(&WordList4, &Word4Ptr->Neighbours, &WordList5);
							
							if (Results5)
							{
								for (int i = 0; i < Results5; i++)
								{
									// Found a solution!
									pthread_mutex_lock(&OutMutex);
									Sequences++;									
									uint32_t Word5Index = WordList5.Neighbour[i];
									printf("%u,%s,%s,%s,%s,%s\n", \
									Sequences,
									Dictionary[Word1Index].Characters, \
									Dictionary[Word2Index].Characters, \
									Dictionary[Word3Index].Characters, \
									Dictionary[Word4Index].Characters, \
									Dictionary[Word5Index].Characters);
									pthread_mutex_unlock(&OutMutex);
								}
							}	
						}
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("Usage: %s <wordlist file> <number of threads>\n", argv[0]);
		return 1;
	}

	FILE *WordList = fopen(argv[1], "r");
	if (WordList == 0)
	{
		fprintf(stderr, "ERROR: Cannot open \"%s\" for reading!\n", argv[optind]);
		return 1;
	}
	
	uint32_t NumThreads = atoi(argv[2]);
	if ((NumThreads == 0) || (NumThreads > 128))
	{
		printf("ERROR: number of threads must be between 1 and 128!\n");
		return 1;
	}
	
	// Start the worker threads, give them time to set up.
	if (pthread_mutex_init(&CalcMutex, NULL)) 
	{
		printf("ERROR: Cannot create calculation mutex!\n");
		return 1;
	}
	if (pthread_mutex_init(&OutMutex, NULL)) 
	{
		printf("ERROR: Cannot create output mutex!\n");
		return 1;
	}

	pthread_t ThreadIDs[128];
	for (uint32_t i = 0; i < NumThreads; i++)
	{
		int Res = pthread_create(&(ThreadIDs[i]), 0, &Worker, 0);
		if (Res != 0)
		{
			printf("ERROR: Can't create thread %u (error %s)\n", i, strerror(Res));
		}
	}
	
	
	// Load the wordlist
	NumWords = 0;
	int Lines = 0;
	Dictionary = malloc(sizeof(Word) * MAXWORDS);
	if (!Dictionary)
	{
		printf("ERROR: Cannot allocate dictionary!\n");
		return 1;
	}	
	while (fgets(Dictionary[NumWords].Characters, 64, WordList))
	{
		Lines++;				
		int BadWord = 0;
		Dictionary[NumWords].LetterMap = 0;		

		// Prune any words that aren't exactly 5 chars
		if (Dictionary[NumWords].Characters[5] == '\r')
		{
			char *Str = Dictionary[NumWords].Characters;
			*(Str+5) = 0;
			for (int j = 0; j < 5; j++)
			{
				char ThisChar = *(Str+j);
				if ((ThisChar < 'a') || (ThisChar > 'z'))
				{
					BadWord = 1;
					printf("ERROR: Wordlist line %u is invalid: \"%s\"\n", Lines, Dictionary[NumWords].Characters);
					printf("Wordlist must be 5 lowercase characters followed by \\r\\n\n");
					return 1;
				}
				else
				{
					
					if (Dictionary[NumWords].LetterMap & (1 << (ThisChar - 'a')))
					{
						// Repeated letter
						BadWord = 1;
						break;
					}
					Dictionary[NumWords].LetterMap |= (1 << (ThisChar - 'a'));
				}
			}					
		}
		else
		{
			BadWord = 1;
		}
		
		if (BadWord == 0)
		{
			// Check for anagrams
			uint32_t MyLetters = Dictionary[NumWords].LetterMap;
			uint8_t Anagram = 0;
			for (int i = 0; i < NumWords; i++)
			{				
				if (Dictionary[i].LetterMap == MyLetters)
				{
					Anagram = 1;
					break;
				}
			}
			if (!Anagram)
			{
				NumWords++;
				if (NumWords == MAXWORDS)
				{
					printf("ERROR: Word list is too long!  Increase MAXWORDS in fivewords.h\n");
					return 1;
				}
			}
		}
	}
	fclose(WordList);
	printf("Loaded %u words with 5 unique letters and no anagrams\n", NumWords);

	// Find the lists of words that could be paired with each word
	for (int Word1 = 0; Word1 < (NumWords-2); Word1++)
	{
		Word *Word1Ptr = &Dictionary[Word1];
		Word1Ptr->Neighbours.NumNeighbours = 0;
		
		for (int Word2 = Word1+1; Word2 < (NumWords-1); Word2++)
		{
			Word *Word2Ptr = &Dictionary[Word2];
			
			if ((Word1Ptr->LetterMap & Word2Ptr->LetterMap) == 0) // Yay for bitfield comparisons!
			{
				// Word2 shares no letters with word1
				Word1Ptr->Neighbours.Neighbour[Word1Ptr->Neighbours.NumNeighbours++] = Word2;
				if (Word1Ptr->Neighbours.NumNeighbours == MAXNEIGHBOURS)
				{
					printf("ERROR: Too many neighbours!  Increase MAXNEIGHBOURS in fivewords.h\n");
					return 1;
				}				
			}
		}
	}	
	
	// And go!
	Running = 1;
		
	// Wait for all the workers to finish
	for (int i = 0; i < NumThreads; i++)
	{
		pthread_join(ThreadIDs[i], NULL);	
	}
}

