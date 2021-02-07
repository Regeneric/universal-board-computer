//  ​Universal Board Computer for cars with electronic MPI
//  Copyright © 2021 IT Crowd, Hubert "hkk" Batkiewicz
// 
//  This file is part of UBC.
//  UBC is free software: you can redistribute it and/or modify
//  ​it under the terms of the GNU Affero General Public License as
//  published by the Free Software Foundation, either version 3 of the
//  ​License, or (at your option) any later version.
// 
//  ​This program is distributed in the hope that it will be useful,
//  ​but WITHOUT ANY WARRANTY; without even the implied warranty of
//  ​MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//  See the ​GNU Affero General Public License for more details.
// 
//  ​You should have received a copy of the GNU Affero General Public License
//  ​along with this program.  If not, see <https://www.gnu.org/licenses/>

// <https://itcrowd.net.pl/>


#include <math.h>
#include "ftoa.h"

// Reverses a string 'str' of length 'len' 
void reverse(char* str, int len) { 
    int i = 0, j = len-1, temp = 0; 
    while(i < j) { 
        temp = str[i]; 
        str[i] = str[j]; 
        str[j] = temp; 
        i++; 
        j--; 
    } 
} 
  
int intToStr(int x, char str[], int d) { 
    int i = 0; 
    while(x) { 
        str[i++] = (x % 10) + '0'; 
        x = x / 10; 
    } 
  
    // If number of digits required is more, then 
    // Add 0s at the beginning 
    while(i < d) str[i++] = '0'; 
  
    reverse(str, i); 
    str[i] = '\0'; 
    return i; 
} 

  
// Converts a floating-point/double number to a string. 
void ftoa(float n, char* res, int afterpoint) { 
    int ipart = (int)n; 
    float fpart = n - (float)ipart;  
    int i = intToStr(ipart, res, 0); 

    if(afterpoint != 0) { 
        res[i] = '.';  // Add dot 
        fpart = fpart * pow(10, afterpoint); 
        intToStr((int)fpart, res + i + 1, afterpoint); 
    } 
} 


// const struct ftoaInterface FTOA = {
//     .convert = ftoa
// };