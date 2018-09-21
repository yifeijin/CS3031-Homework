#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#define MAX_BYTES_OF_READ	(8192)	/* max bytes of function read() */
#define MAX_THREADS_NUM		(16)	/* max threads number */


char *gSourceFile  = NULL;	/* source file name */
char *gSearchStr   = NULL;	/* searching string */
int  gWay          = 0;		/* the way of file I/O, 0 - read(), 1 - mmap() 2 - multiple thread*/
int  gBytes        = 0;		/* bytes of  function read() */
int  gThreadsNum   = 0;		/* number of threads */

unsigned long gSize = -1;	/* bytes of source file */
int gRestBytes      = 0;	/* last bytes should be compared next time */


void ParseParams(int argc, char *argv[])
{
	char *pTemp = NULL;
	
	gSourceFile  = argv[1];
	gSearchStr   = argv[2];
	
	if (argc < 4)
	{
		gWay = 0;
	}
	else
	{
		if (0 == strcmp(argv[3], "mmap"))
		{
			gWay = 1;
		}
		else if ((pTemp = strstr(argv[3], "p")) != NULL)
		{
			gWay        = 2;
			gThreadsNum = atoi(pTemp + 1);
		}
		else
		{
			gWay   = 0;
			gBytes = atoi(argv[3]);
		}
	}

	if ((NULL == gSourceFile) ||(NULL == gSearchStr))
	{
		printf("error: sourece file and searching string should not be empty.\n");
		exit(-1);
	}
	if (gBytes > MAX_BYTES_OF_READ)
	{
		printf("error: bytes should not greater than %d.\n", MAX_BYTES_OF_READ);
		exit(-1);
	}
	if (gThreadsNum > MAX_THREADS_NUM)
	{
		printf("error: threads number should not greater than %d.\n", MAX_THREADS_NUM);
		exit(-1);
	}
}


int GetFileSize(void)
{  
	unsigned long uiSize = -1; 
	FILE          *pFp   = NULL;  
	
	pFp = fopen(gSourceFile, "r");	
	if (NULL == pFp)	
	{
		return uiSize;  
	}
	
	fseek(pFp, 0L, SEEK_END);  
	uiSize = ftell(pFp);  
	
	fclose(pFp);  
	return uiSize;  
} 


int SearchStr(char *pSrc, unsigned long uiSize)
{
	if (NULL == pSrc)
	{
		return 0;
	}

	int  i;
	int  iTimes      = 0;
	char *pSearchStr = gSearchStr;
	
	for (i = 0; i < gRestBytes; i ++)	/* compare rest bytes first */
	{
		if (pSrc[i] != *(pSearchStr + gRestBytes + 1))
		{
			break;
		}
		pSearchStr++;
		if (*pSearchStr == '\0')
		{
			iTimes ++;
			break;
		}
	}

	pSearchStr = gSearchStr;
	for (i = 0; i < uiSize; i ++)
	{
		if (pSrc[i] == *pSearchStr)
		{
			pSearchStr++;
			if ((pSearchStr - gSearchStr) == strlen(gSearchStr))
			{
				iTimes++;
			}
		}
		else
		{
			if (pSearchStr != gSearchStr)
			{
				pSearchStr = gSearchStr;
				i -= (pSearchStr - gSearchStr);
			}
		}
	}

	if ((pSearchStr - gSearchStr) != strlen(gSearchStr))
	{
		gRestBytes = pSearchStr - gSearchStr;
	}
	else
	{
		gRestBytes = 0;
	}

	return iTimes;
}


void *Thread_Func(void *pBuffer)
{
	unsigned char *pTemp = (unsigned char *)pBuffer;
	int *pTime = (int *)malloc(sizeof(int));

	*pTime = SearchStr(pTemp, gSize / 4);
	return pTime;
}


int Search_Read()
{
	int           iFd          = 0;
	int           iOccurrences = 0;
	unsigned char *pBuffer     = NULL;
	unsigned long uiSize       = 0;

	if (0 == gBytes)	/* if gBytes is not been seted by user, we set it to 1024 */
	{
		gBytes = 1024;
	}
	pBuffer = (char *)malloc(sizeof(unsigned char) * gBytes);
	memset(pBuffer, 0, sizeof(unsigned char) * gBytes);

	iFd = open(gSourceFile, O_RDONLY);
	while ((uiSize = read(iFd, pBuffer, gBytes)) != 0)
	{
		iOccurrences += SearchStr(pBuffer, uiSize);
		memset(pBuffer, 0, sizeof(unsigned char) * gBytes);
	}

	free(pBuffer);
	return iOccurrences;
}


int Search_Mmap()
{
	int           iFd          = 0;
	int           iOccurrences = 0;
	unsigned char *pBuffer     = NULL;
	
	iFd     = open(gSourceFile, O_RDONLY);
	pBuffer = (unsigned char *)mmap(NULL, gSize, PROT_READ, MAP_SHARED, iFd, 0);
	
	iOccurrences = SearchStr(pBuffer, gSize);
	
	munmap(pBuffer, gSize);
	return iOccurrences;
}


int Search_MmapThreads()
{
	int           iFd          = 0;
	int           iOccurrences = 0;
	unsigned char *pBuffer     = NULL;
	int           i            = 0;
	pthread_t     iThread[MAX_THREADS_NUM];
	void          *pTime;
	
	iFd     = open(gSourceFile, O_RDONLY);
	pBuffer = (unsigned char *)mmap(NULL, gSize, PROT_READ, MAP_SHARED, iFd, 0);

	for (i = 0; i < gThreadsNum; i++)
	{
		pthread_create(&iThread[i], NULL, Thread_Func, pBuffer + i * (gSize / 4));
		pthread_join(iThread[i], &pTime);
		iOccurrences += (int)*((int *)pTime);
	}
	
	free(pTime);
	munmap(pBuffer, gSize);
	return iOccurrences;
}


int main(int argc, char *argv[])
{
	int i = 0;

	ParseParams(argc, argv);

	gSize = GetFileSize();
	printf("File size: %ld bytes.\n", gSize);

	if (0 == gWay)
	{
		printf("Occurrences of the string \"%s\": %d\n", gSearchStr, Search_Read());
	}
	else if (1 == gWay)
	{
		printf("Occurrences of the string \"%s\": %d\n", gSearchStr, Search_Mmap());
	}
	else
	{
		printf("Occurrences of the string \"%s\": %d\n", gSearchStr, Search_MmapThreads());
	}

	return 0;
}
