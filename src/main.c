#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOTAL_PRODUCTS 50

// 商品情報の構造体
typedef struct{
    int id;     // 商品ID
    char name[50];      // 商品名
    int price;  // 価格
    int stock;  // 在庫
} Product;

// 自販機システム管理の構造体
typedef struct{
    int current;    // 消費者が投入した金額
    int total;      // 売上金額
} VendingMachine;

typedef struct{
    STATE_INSERT_MONEY;
    STATE_PRINT_MENU;
    STATE_SELECT_MENU;
    STATE_DISPENSE_PRODUCT;
    STATE_GIVE_CHANGE;
} VendingState;

int main(void)
{
	printf("=== メニュー ===\n");
	printf("1: 機能A\n");
	printf("9: 終了\n");
	return 0;
} 