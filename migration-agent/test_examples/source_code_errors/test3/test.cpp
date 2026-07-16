#include <stdio.h>
// #include "file.h"
int abc(char* a, char* b, bool c){
return 1;
}
int abc(bool a, bool b, char* c){
return 2;
}
int main(){
printf("%d\n",abc("his" ,"sophic"));
return 0;
}
