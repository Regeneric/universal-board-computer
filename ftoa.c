#include <math.h>
#include "ftoa.h"

// Reverses a string 'str' of length 'len' 
void reverse(char* str, int len) { 
    int i = 0, j = len-1, temp = 0; 
    while (i < j) { 
        temp = str[i]; 
        str[i] = str[j]; 
        str[j] = temp; 
        i++; 
        j--; 
    } 
} 
  
int intToStr(int x, char str[], int d) { 
    int i = 0; 
    while (x) { 
        str[i++] = (x % 10) + '0'; 
        x = x / 10; 
    } 
  
    // If number of digits required is more, then 
    // add 0s at the beginning 
    while (i < d) 
        str[i++] = '0'; 
  
    reverse(str, i); 
    str[i] = '\0'; 
    return i; 
} 

  
// Converts a floating-point/double number to a string. 
void ftoa(float n, char* res, int afterpoint) { 
    int ipart = (int)n; 
    float fpart = n - (float)ipart;  
    int i = intToStr(ipart, res, 0); 

    if (afterpoint != 0) { 
        res[i] = '.'; // add dot 
        fpart = fpart * pow(10, afterpoint); 
        intToStr((int)fpart, res + i + 1, afterpoint); 
    } 
} 


const struct ftoaInterface FTOA = {
    .convert = ftoa
};