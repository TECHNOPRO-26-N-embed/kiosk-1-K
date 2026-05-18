#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOTAL_PRODUCTS 50

// 金額投入の関数
int TonyuKingaku(int moto) {
    int KingakuChoice;
    printf("投入金額をえらんでください。\n");
    printf("1: 10円\n");
    printf("2: 50円\n");
    printf("3: 100円\n");
    printf("4: 500円\n");
    printf("5: 1000円\n");
    printf("6: 投入を終了する\n");
        while (moto < 2000) {
            scanf("%d", &KingakuChoice);          
                if(KingakuChoice == 1) {
                    moto += 10;
                    printf("現在の投入金額: %d円\n", moto);
                }
                if(KingakuChoice == 2) {
                    moto += 50;
                    printf("現在の投入金額: %d円\n", moto);
                }
                if(KingakuChoice == 3) {
                    moto += 100;
                    printf("現在の投入金額: %d円\n", moto);
                }
                if(KingakuChoice == 4) {
                    moto += 500;
                    printf("現在の投入金額: %d円\n", moto);
                }
                if(KingakuChoice == 5) {
                    moto += 1000;
                    printf("現在の投入金額: %d円\n", moto);
                if (KingakuChoice == 6){
                    break;
                }
                }else{
                printf("現在投入金額: %d円\n", moto);
                }
            }
    return moto;
}

int Inventory_management(int product_id, int quantity) {
    // 商品の在庫管理のロジックをここに実装
    
    return 0;
}

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

void loadMenu(Product menu[]);
void printMenu(Product menu[], int balance);
void saveCSV(Product purchased_item);


int main(void)
{
	Product menu[TOTAL_PRODUCTS];
    loadMenu(menu);
    printf("合計金額: %d円\n", TonyuKingaku(0));

    
    return 0;
} 

