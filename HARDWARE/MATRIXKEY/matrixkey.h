#ifndef __MATIRXKEY_H
#define __MATIRXKEY_H	

#include "sys.h" 
#include "delay.h" 


typedef struct{
		int key[8][8];//8*8的矩阵，其实只用到了7-5，存储方块的状态
		/*int block11;
		int block41;
		int block71;
		int block32;
		int block52;
		int block23;
		int block63;
		int block34;
		int block54;
		int block15;
		int block45;
		int block75;
		int key00;
		int key73;
		int key26;*/
} keystate;

void MATRI4X4KEY_Init(void);
int MATRI4X4KEY_Scan(int * row, int * column);
int MATRI4_4KEY_Scan(int * row, int * column);
//int equal2(int row,int column,int expect_row,int expect_column);
//int Parse_Key(int row,int column);
#endif
