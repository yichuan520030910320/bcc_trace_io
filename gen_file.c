#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int targetSize = 0; //size unit is byte
    char targetSizeStr[10];  //size unit is K / M
    
strcpy(targetSizeStr,argv[1]);

    // printf("please enter the target size! size unit is K or M; you need to enter the unit and care the spelling of the unit\n");
    // scanf("%s",targetSizeStr);
    
    FILE *file = fopen("in.txt","w+");
    
    if(file == NULL)
    {
        printf("open error!\n");
        return 0;
    }
    int length = strlen(targetSizeStr);
    int i = 0,j = 10;
    for (i = 0; i < length; i++)
    {
    	if (targetSizeStr[i] == 'K')
    	{
    	    targetSize *= 1024;
    	    break;
    	}
    	if (targetSizeStr[i] == 'M')
    	{
    	    targetSize *= (1024 * 1024);
    	    break;
    	}
    	int number = targetSizeStr[i] - '0';
    	targetSize = targetSize * j + number;
    }
    printf("target size ;%d ",targetSize);
    
    for (i = 0; i < targetSize; i++)
    {
    	fprintf(file,"%c",'0');
    }
    
    fclose(file);
    
    return 0;
}
