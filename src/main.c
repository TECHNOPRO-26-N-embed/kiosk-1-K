#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOTAL_PRODUCTS 50

// 金額投入の関数
int TonyuKingaku(int moto) {
    
    int KingakuChoice;

    while (moto < 10000) {

        printf("\n===============================\n");
        printf("[現在投入残高: %d円]\n", moto);
        printf("投入金額をえらんでください。\n");
        printf("1: 10円\n");
        printf("2: 50円\n");
        printf("3: 100円\n");
        printf("4: 500円\n");
        printf("5: 1000円\n");
        printf("6: 投入を終了する\n");
        printf("===============================\n");
        printf("選択: ");

        if(scanf("%d", &KingakuChoice) != 1){
            printf("無効な入力\n");
            while(getchar() != '\n');
            continue;
        }

        if(KingakuChoice == 1) moto += 10;
        else if(KingakuChoice == 2) moto += 50;
        else if(KingakuChoice == 3) moto += 100;
        else if(KingakuChoice == 4) moto += 500;
        else if(KingakuChoice == 5) moto += 1000;
        else if(KingakuChoice == 6) exit(0);
        else {
            printf("無効な番号\n");
            continue;
        }

        printf("現在の投入金額: %d円\n", moto);
    }

    return moto;

}

// スタートメニューを追加0518(岩野)
void StartMenu(){
    int inputKey; // 押された数字
    int price = 100; // 適当な値段

    for(int i = 0; i < TOTAL_PRODUCTS; i++) {
        // 商品の情報を表示
            printf("%dジュース:%d円 ", i + 1, price);

        // 5個ごとに改行する
        if(i % 5 == 4) {
            printf("\n");
         }
     }

    printf("1を押すと金額投入へ\n");
    printf("2を押すと終了\n");

     // ユーザーの入力を受け取る
     scanf("%d", &inputKey);

     if(inputKey == 1){ // 1が押されたら
        // 金額投入の関数を呼び出す
        int TotalMoney = 0;
        int moto = 0;
        TotalMoney = TonyuKingaku(moto);
        printf("投入金額: %d円\n", TotalMoney);
     }else if(inputKey != 1){ // 1以外が押されたら
        printf("終了します。\n");
        exit(0);
  }
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
    if (menu[product_id].stock > 0)// 在庫がある場合のみ減らす
    {
        menu[product_id].stock -= 1; 
        printf("商品ID %d の在庫が減りました。残り在庫: %d\n", product_id, menu[product_id].stock);
    }
    else if(menu[product_id].stock == 0)
    {
        // 在庫がない場合の処理(売り切れ表示をするなど)
        printf("商品ID %d の在庫がありません。\n", product_id);
    }
}

int main(void)
{
	Product menu[TOTAL_PRODUCTS];
    //loadMenu(menu);

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

    //スタートメニュー呼び出し0518(岩野)
    StartMenu();

    while(1)
    {
        switch (current_state)
        {
            case STATE_INSERT_MONEY:
                
                machine.current = TonyuKingaku(machine.current);

                if(machine.current > 0){
                    current_state = STATE_PRINT_MENU;
                } else {
                    printf("お金が投入されていません\n");
                }

                break;

            // メニュー表示と選択の処理
            case STATE_PRINT_MENU:
                //printMenu(menu, machine.current);
                current_state = STATE_SELECT_MENU;
                break;

            case STATE_SELECT_MENU:
                printf("購入する商品番号(1~%d)を選んでください（お金をもっと入れる場合は0を入力）:\n", TOTAL_PRODUCTS);

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
                    break;
                }
                if(machine.current < menu[target_index].price)
                {
                    printf("残高が不足しています。%sの価格は%d円ですが、現在の残高は%d円です。\n", menu[target_index].name, menu[target_index].price, machine.current);
                    break;
                }
                
                current_state = STATE_DISPENSE_PRODUCT;
                break;
                
                // 商品提供と在庫管理の処理
                case STATE_DISPENSE_PRODUCT:
                    printf("%sを提供しています...\n", menu[target_index].name);
                    machine.current -= menu[target_index].price;
                    machine.total += menu[target_index].price;
                    Inventory_management(target_index, menu);
                    //saveCSV(menu[target_index]);
                    printf("%sを購入しました。残高: %d円\n", menu[target_index].name, machine.current);
                    current_state = STATE_PRINT_MENU;
                    break;

                // お釣り返却の処理
                case STATE_GIVE_CHANGE:
                    printf("お釣りを返しています...\n");
                    printf("お釣り: %d円\n", machine.current);
                    int changeReturn = machine.current;

                    // お釣りを返すためのループ
                    int return_1000 = 0, return_500 = 0, return_100 = 0, return_50 = 0, return_10 = 0;
                    while(changeReturn >= 1000 && machine.bill_1000 > 0)
                    {
                        changeReturn -= 1000;
                        machine.bill_1000--;
                        return_1000++;
                    }
                    while(changeReturn >= 500 && machine.coin_500 > 0)
                    {
                        changeReturn -= 500;
                        machine.coin_500--;
                        return_500++;
                    }
                    while(changeReturn >= 100 && machine.coin_100 > 0)
                    {
                        changeReturn -= 100;
                        machine.coin_100--;
                        return_100++;
                    }
                    while(changeReturn >= 50 && machine.coin_50 > 0)
                    {
                        changeReturn -= 50;
                        machine.coin_50--;
                        return_50++;
                    }
                    while(changeReturn >= 10 && machine.coin_10 > 0)
                    {
                        changeReturn -= 10;
                        machine.coin_10--;
                        return_10++;
                    }

                    // 返却するお釣りの内訳を表示
                    if(return_1000 > 0) printf("お釣り: %d円\n", machine.current);
                    if(return_1000 > 0) printf("1000円札: %d枚\n", return_1000);
                    if(return_500 > 0) printf("500円硬貨: %d枚\n", return_500);
                    if(return_100 > 0) printf("100円硬貨: %d枚\n", return_100);
                    if(return_50 > 0) printf("50円硬貨: %d枚\n", return_50);
                    if(return_10 > 0) printf("10円硬貨: %d枚\n", return_10);

                    // お釣りを完全に返せなかった場合の対応
                    if(changeReturn > 0)
                    {
                        printf("申し訳ありませんが、十分なお釣りを返すことができません。残りのお釣り: %d円\n", changeReturn);
                        printf("管理者にお問合せください。\n");
                    }
                    else{
                        printf("お釣りを返しました。ありがとうございました！\n");
                    }
                    
                    machine.current = 0; // 残高をリセット
                    current_state = STATE_INSERT_MONEY;
                    break;
        }
    }
    
    return 0;
} 