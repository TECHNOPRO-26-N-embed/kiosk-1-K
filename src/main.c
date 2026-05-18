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
    
    // 硬貨と紙幣の保有枚数
    int coin_10;    // 10円硬貨の枚数
    int coin_50;    // 50円硬貨の枚数
    int coin_100;   // 100円硬貨の枚数
    int coin_500;   // 500円硬貨の枚数
    int bill_1000;  // 1000円札の枚数
} VendingMachine;

// 自販機の状態を表す構造体
typedef struct{
    STATE_INSERT_MONEY;     // お金の投入
    STATE_PRINT_MENU;       // メニューの表示
    STATE_SELECT_MENU;      // メニューの選択
    STATE_DISPENSE_PRODUCT; // 商品の提供
    STATE_GIVE_CHANGE;      // お釣りの提供
} VendingState;

int main(void)
{
	printf("=== メニュー ===\n");
	printf("1: 機能A\n");
	printf("9: 終了\n");
	return 0;
} 