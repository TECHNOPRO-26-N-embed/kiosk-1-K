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

    // 硬貨と紙幣の在庫
    int coin_10;    // 10円硬貨の在庫
    int coin_50;    // 50円硬貨の在庫
    int coin_100;   // 100円硬貨の在庫
    int coin_500;   // 500円硬貨の在庫
    int bill_1000;  // 1000円紙幣の在庫
} VendingMachine;

// 自販機の状態を表す列挙型
typedef enum {
    STATE_INSERT_MONEY,     // お金を投入する状態
    STATE_PRINT_MENU,       // メニューを表示する状態
    STATE_SELECT_MENU,      // メニューを選択する状態
    STATE_DISPENSE_PRODUCT, // 商品を提供する状態
    STATE_GIVE_CHANGE   // お釣りを返す状態
} VendingState;

void loadMenu(Product menu[]);
void printMenu(Product menu[], int balance);
void saveCSV(Product purchased_item);

void Inventory_management(int product_id, Product menu[]) {
    // 商品の在庫管理のロジックをここに実装
    // 例: 在庫を減らす
    menu[product_id].stock -= 1; 
}


int main(void)
{
	Product menu[TOTAL_PRODUCTS];
    loadMenu(menu);

    VendingMachine machine = {
        .current = 0,
        .total = 0,
        .coin_10 = 100,
        .coin_50 = 100,
        .coin_100 = 100,
        .coin_500 = 100,
        .bill_1000 = 100
    };

    VendingState current_state = STATE_INSERT_MONEY;

    int button_pressed = 0;
    int user_input = 0;
    int target_index = -1;

    printf("自動販売機へようこそ！\n");

    while(1)
    {
        switch (current_state)
        {
            case STATE_INSERT_MONEY:
                printf("\n===============================\n");
                printf("[現在投入残高: %d円] (※最大10000円まで)\n", machine.current);
                printf("投入ボタンを選んでください:\n");
                printf("1: [10円] 2: [50円] 3: [100円] 4: [500円] 5: [1000円] 6: [投入終了]\n");
                printf("===============================\n");
                printf("選択: ");

                if(scanf("%d", &button_pressed) != 1)
                {
                    printf("無効な入力です。もう一度選択してください。\n");
                    while(getchar() != '\n'); // 入力バッファをクリア
                    continue;
                }

                int added_amount = 0;

                switch (button_pressed)
                {
                    case 1: added_amount = 10; break;
                    case 2: added_amount = 50; break;
                    case 3: added_amount = 100; break;
                    case 4: added_amount = 500; break;
                    case 5: added_amount = 1000; break;

                    case 6: 
                        if(machine.current == 0)
                        {
                            printf("お金を先に入れてください。\n");
                        }
                        else
                        {
                            current_state = STATE_PRINT_MENU;
                        }
                        
                        continue;
                    case 7:
                        current_state = STATE_GIVE_CHANGE;
                        continue;
                    default:
                        printf("無効な選択です。もう一度選択してください。\n");
                        continue;
                }
                if(machine.current + added_amount > 10000)
                {
                    printf("これ以上投入できません。最大投入金額は10000円です。\n");
                }
                else
                {
                    machine.current += added_amount;
                    printf("%d円を投入しました。現在の残高: %d円\n", added_amount, machine.current);
                }
                break;

                case STATE_PRINT_MENU:
                    printMenu("購入する商品番号(1~%d)を選んでください（お金をもっと入れる場合は0を入力）:\n", TOTAL_PRODUCTS);

                    if(scanf("%d", &user_input) != 1)
                    {
                        printf("無効な入力です。もう一度選択してください。\n");
                        while(getchar() != '\n'); // 入力バッファをクリア
                        break;
                    }

                    if(user_input == 0)
                    {
                        current_state = STATE_INSERT_MONEY;
                        break;
                    }
                    else if(user_input < 1 || user_input > TOTAL_PRODUCTS)
                    {
                        printf("無効な商品番号です。もう一度選択してください。\n");
                    }
                    
                    target_index = user_input - 1; // 商品番号は1から始まるため、インデックスは-1する

                    if(menu[target_index].stock <= 0)
                    {
                        printf("申し訳ありませんが、%sは在庫切れです。\n", menu[target_index].name);
                    }
                    if(machine.current < menu[target_index].price)
                    {
                        printf("残高が不足しています。%sの価格は%d円ですが、現在の残高は%d円です。\n", menu[target_index].name, menu[target_index].price, machine.current);
                    }
                    
                    current_state = STATE_DISPENSE_PRODUCT;
                    
                    break;
                    current_state = STATE_SELECT_MENU;
                    break;
        }
    }
    
    return 0;
} 