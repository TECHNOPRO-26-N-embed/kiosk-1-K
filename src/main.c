#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_PRODUCTS 50
#define MAX_NAME_LEN 64
#define MAX_STOCK 50
#define MAX_BALANCE 5000
#define DENOM_COUNT 5
#define INPUT_BUF 256
#define PRODUCT_COLS 5
#define PRODUCT_CELL_WIDTH 14

#define PRODUCT_CSV_FILE "docs/menu.csv"
#define LOG_CSV_FILE "data/data.csv"

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    int price;
    int stock;
    int max_stock;
    int temperature;
} Product;

typedef struct {
    char timestamp[20];
    int product_id;
    char product_name[MAX_NAME_LEN];
    int quantity;
    int unit_price;
    int total_price;
    int inserted_amount;
    int change_amount;
} SaleData;

typedef struct {
    char timestamp[20];
    char action[64];
    char detail[128];
} OperationLog;

typedef struct {
    char timestamp[20];
    char error_code[32];
    char detail[128];
} ErrorLog;

typedef struct {
    char timestamp[20];
    char fault_type[64];
    int risk_score;
    char detail[128];
} FaultPredictionLog;

typedef struct {
    int balance;
    int selected_product_id;
    int quantity;
    int total_price;
    int active;
} TransactionState;

/* 商品情報、自販機内金種、取引状態を保持するグローバル変数。 */
static Product g_products[MAX_PRODUCTS];
static int g_machine_money[DENOM_COUNT] = {30, 20, 30, 20, 10};
static const int g_denominations[DENOM_COUNT] = {10, 50, 100, 500, 1000};
static TransactionState g_tx;
static SaleData g_last_sale;
static int g_has_last_sale = 0;
static int g_shutdown_requested = 0;

/* 詳細設計書で定義された関数宣言。 */
void init_products(void);
int load_products_from_csv(const char *filename);
int save_products_to_csv(const char *filename);
int is_sold_out(int product_id);
int reduce_stock(int product_id, int quantity);
int restock_product(int product_id, int quantity);

void show_main_menu(void);
void show_product_list(void);
void show_current_balance(int balance);
int read_int_safe(void);
int input_product_no(void);
int input_quantity(void);

int *input_money_counts(void);
int calc_inserted_amount(int money_counts[]);
void add_money_to_machine(int money_counts[]);
int limit_max_money(int balance);
int can_afford(int balance, int price);
int calc_total_price(int unit_price, int quantity);
int calc_change_amount(int balance, int price);
int can_make_change(int change_amount);
int *make_change_breakdown(int change_amount);
void apply_change_payout(int change_counts[]);

void start_transaction(void);
int process_purchase(int product_id, int quantity, int balance);
void cancel_transaction(int balance);
void finish_transaction(void);
void reset_for_next_customer(void);

int log_sale_csv(const char *filename, SaleData sale);
int log_money_csv(const char *filename, int money_counts[]);
int log_operation_csv(const char *filename, OperationLog log);
int log_error_csv(const char *filename, ErrorLog log);
int log_fault_prediction_csv(const char *filename, FaultPredictionLog log);
int ensure_csv_headers(const char *filename, const char *headers);

void run_vending_machine(void);
void handle_admin_menu(void);

/* 現在購入可能な商品の最安価格を返す。購入不能時は-1を返す。 */
static int get_min_available_price(void);

/* 取引終了時に残高を返金し、取引状態を終了させる。 */
static void payout_and_end_transaction(const char *reason);

/* 固定幅セル内に文字列を中央寄せで表示する。 */
static void print_center_cell(const char *text, int width);

/* UTF-8先頭バイトから1文字のバイト数を判定する。 */
static int utf8_char_bytes(unsigned char lead);

/* UTF-8文字列の表示幅（半角1/全角2）を返す。 */
static int utf8_display_width(const char *s);

/* UTF-8文字列を表示幅 max_width 以内で切り詰める。 */
static void utf8_truncate_display_width(const char *src, int max_width, char *dst, size_t dst_size);

/* 00入力による終了要求状態を返す。 */
static int is_shutdown_requested(void);

/* WindowsコンソールをUTF-8入出力へ設定する。 */
static void configure_console_utf8(void) {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
}

/* ログ・CSV出力用の現在時刻文字列を生成する。 */
static void get_now_timestamp(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    if (tm_now == NULL) {
        snprintf(buf, size, "1970-01-01 00:00:00");
        return;
    }
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_now);
}

/* ファイルから読んだ行末の改行コードを取り除く。 */
static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

/* 現在時刻付きの操作ログ構造体を生成する。 */
static OperationLog make_operation_log(const char *action, const char *detail) {
    OperationLog log;
    get_now_timestamp(log.timestamp, sizeof(log.timestamp));
    snprintf(log.action, sizeof(log.action), "%s", action);
    snprintf(log.detail, sizeof(log.detail), "%s", detail);
    return log;
}

/* 現在時刻付きのエラーログ構造体を生成する。 */
static ErrorLog make_error_log(const char *code, const char *detail) {
    ErrorLog log;
    get_now_timestamp(log.timestamp, sizeof(log.timestamp));
    snprintf(log.error_code, sizeof(log.error_code), "%s", code);
    snprintf(log.detail, sizeof(log.detail), "%s", detail);
    return log;
}

/* 異常傾向を記録する故障予兆ログ構造体を生成する。 */
static FaultPredictionLog make_fault_log(const char *type, int score, const char *detail) {
    FaultPredictionLog log;
    get_now_timestamp(log.timestamp, sizeof(log.timestamp));
    snprintf(log.fault_type, sizeof(log.fault_type), "%s", type);
    log.risk_score = score;
    snprintf(log.detail, sizeof(log.detail), "%s", detail);
    return log;
}

/* 自販機内の金種在庫で釣銭を作れるかシミュレーションする（10/50/100/500のみ）。 */
static int simulate_change_breakdown(int change_amount, int out_counts[DENOM_COUNT]) {
    int remaining = change_amount;
    int i;

    for (i = 0; i < DENOM_COUNT; i++) {
        out_counts[i] = 0;
    }

    for (i = DENOM_COUNT - 2; i >= 0; i--) {
        int use = 0;
        if (g_denominations[i] > 0) {
            int need = remaining / g_denominations[i];
            use = (need < g_machine_money[i]) ? need : g_machine_money[i];
            out_counts[i] = use;
            remaining -= use * g_denominations[i];
        }
    }

    return (remaining == 0) ? 1 : 0;
}

/* 商品50種類の初期値（名称・価格・在庫）を設定する。 */
void init_products(void) {
    int i;
    for (i = 0; i < MAX_PRODUCTS; i++) {
        g_products[i].id = i + 1;
        snprintf(g_products[i].name, sizeof(g_products[i].name), "商品%02d", i + 1);
        g_products[i].price = 100 + (i % 10) * 10;
        g_products[i].stock = MAX_STOCK;
        g_products[i].max_stock = MAX_STOCK;
        g_products[i].temperature = (i % 2 == 0) ? 5 : 55;
    }
}

/* CSVから商品マスタと在庫情報を読み込む。 */
int load_products_from_csv(const char *filename) {
    FILE *fp = fopen(filename, "r");
    char line[INPUT_BUF];
    int count = 0;

    if (fp == NULL) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL && count < MAX_PRODUCTS) {
        Product p;
        trim_newline(line);
        if (line[0] == '\0') {
            continue;
        }
        if (strncmp(line, "id,", 3) == 0) {
            continue;
        }

        if (sscanf(line, "%d,%63[^,],%d,%d,%d,%d",
                   &p.id,
                   p.name,
                   &p.price,
                   &p.stock,
                   &p.max_stock,
                   &p.temperature) == 6) {
            if (p.id >= 1 && p.id <= MAX_PRODUCTS) {
                g_products[p.id - 1] = p;
                count++;
            }
        }
    }

    fclose(fp);
    return (count > 0) ? 0 : -1;
}

/* 現在の商品情報と在庫をCSVに保存する。 */
int save_products_to_csv(const char *filename) {
    FILE *fp = fopen(filename, "w");
    int i;

    if (fp == NULL) {
        return -1;
    }

    fprintf(fp, "id,name,price,stock,max_stock,temperature\n");
    for (i = 0; i < MAX_PRODUCTS; i++) {
        fprintf(fp,
                "%d,%s,%d,%d,%d,%d\n",
                g_products[i].id,
                g_products[i].name,
                g_products[i].price,
                g_products[i].stock,
                g_products[i].max_stock,
                g_products[i].temperature);
    }

    fclose(fp);
    return 0;
}

/* 指定商品の在庫が売り切れかどうかを判定する。 */
int is_sold_out(int product_id) {
    if (product_id < 1 || product_id > MAX_PRODUCTS) {
        return 1;
    }
    return (g_products[product_id - 1].stock <= 0) ? 1 : 0;
}

/* 購入数量分の在庫を減算する。 */
int reduce_stock(int product_id, int quantity) {
    Product *p;
    if (product_id < 1 || product_id > MAX_PRODUCTS || quantity <= 0) {
        return -1;
    }

    p = &g_products[product_id - 1];
    if (p->stock < quantity) {
        return -1;
    }

    p->stock -= quantity;
    return 0;
}

/* 在庫上限を超えない範囲で商品在庫を補充する。 */
int restock_product(int product_id, int quantity) {
    Product *p;
    if (product_id < 1 || product_id > MAX_PRODUCTS || quantity <= 0) {
        return -1;
    }

    p = &g_products[product_id - 1];
    if (p->stock + quantity > p->max_stock) {
        p->stock = p->max_stock;
    } else {
        p->stock += quantity;
    }
    return 0;
}

/* 購入処理と管理処理のメインメニューを表示する。 */
void show_main_menu(void) {
    printf("\n=== メインメニュー ===\n");
    printf("1. 購入を開始する\n");
    printf("0. 終了\n");
    printf("00. 即時終了\n");
    printf("選択してください: ");
}

/* 売り切れ状態を含む商品一覧を表示する。 */
void show_product_list(void) {
    int row;
    for (row = 0; row < (MAX_PRODUCTS / PRODUCT_COLS); row++) {
        int col;
        char buf[32];

        for (col = 0; col < PRODUCT_COLS; col++) {
            int idx = row * PRODUCT_COLS + col;
            print_center_cell(g_products[idx].name, PRODUCT_CELL_WIDTH);
        }
        printf("\n");

        for (col = 0; col < PRODUCT_COLS; col++) {
            int idx = row * PRODUCT_COLS + col;
            snprintf(buf, sizeof(buf), "%d", g_products[idx].id);
            print_center_cell(buf, PRODUCT_CELL_WIDTH);
        }
        printf("\n");

        for (col = 0; col < PRODUCT_COLS; col++) {
            int idx = row * PRODUCT_COLS + col;
            snprintf(buf, sizeof(buf), "%d", g_products[idx].price);
            print_center_cell(buf, PRODUCT_CELL_WIDTH);
        }
        printf("\n");

        for (col = 0; col < PRODUCT_COLS; col++) {
            int idx = row * PRODUCT_COLS + col;
            print_center_cell((g_products[idx].stock <= 0) ? "売り切れ" : "販売中", PRODUCT_CELL_WIDTH);
        }
        printf("\n\n");
    }
}

/* 現在の投入残高を表示する。 */
void show_current_balance(int balance) {
    printf("現在残高: %d 円\n", balance);
}

/* 整数入力を安全に受け取り、不正入力時は再入力させる。 */
int read_int_safe(void) {
    char buf[INPUT_BUF];
    char *endptr;
    long v;

    while (1) {
        if (is_shutdown_requested()) {
            return 0;
        }

        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            g_shutdown_requested = 1;
            return 0;
        }
        trim_newline(buf);

        if (strcmp(buf, "00") == 0) {
            g_shutdown_requested = 1;
            return 0;
        }

        if (buf[0] == '\0') {
            printf("数値を入力してください（00で終了）: ");
            continue;
        }

        v = strtol(buf, &endptr, 10);
        if (*endptr == '\0') {
            return (int)v;
        }

        printf("不正な入力です。再入力してください（00で終了）: ");
    }
}

/* 商品番号入力を受け取り、範囲と売り切れ状態を検証する。 */
int input_product_no(void) {
    int product_id;
    while (1) {
        printf("商品番号を入力してください (1-%d / 00で終了): ", MAX_PRODUCTS);
        product_id = read_int_safe();
        if (is_shutdown_requested()) {
            return 0;
        }
        if (product_id < 1 || product_id > MAX_PRODUCTS) {
            printf("範囲外です。\n");
            continue;
        }
        if (is_sold_out(product_id)) {
            printf("選択した商品は売り切れです。\n");
            continue;
        }
        return product_id;
    }
}

/* 購入数量入力を受け取り、在庫範囲内かを検証する。 */
int input_quantity(void) {
    int quantity;
    int product_id = g_tx.selected_product_id;

    while (1) {
        printf("購入数量を入力してください (1-%d / 00で終了): ", g_products[product_id - 1].stock);
        quantity = read_int_safe();
        if (is_shutdown_requested()) {
            return 0;
        }
        if (quantity < 1) {
            printf("数量は1以上にしてください。\n");
            continue;
        }
        if (quantity > g_products[product_id - 1].stock) {
            printf("在庫数を超えています。\n");
            continue;
        }
        return quantity;
    }
}

/* 各金種の投入枚数を受け取り、配列で返す。 */
int *input_money_counts(void) {
    static int counts[DENOM_COUNT];
    int i;

    for (i = 0; i < DENOM_COUNT; i++) {
        printf("%d円の投入枚数を入力してください（00で終了）: ", g_denominations[i]);
        counts[i] = read_int_safe();
        if (is_shutdown_requested()) {
            return counts;
        }
        if (counts[i] < 0) {
            counts[i] = 0;
        }
    }
    return counts;
}

/* 金種別の投入枚数から合計投入金額を計算する。 */
int calc_inserted_amount(int money_counts[]) {
    int i;
    int total = 0;
    for (i = 0; i < DENOM_COUNT; i++) {
        total += money_counts[i] * g_denominations[i];
    }
    return total;
}

/* 投入された金種枚数を自販機内の保有枚数へ加算する。 */
void add_money_to_machine(int money_counts[]) {
    int i;
    for (i = 0; i < DENOM_COUNT; i++) {
        g_machine_money[i] += money_counts[i];
    }
}

/* 残高が最大投入可能額を超えていないか判定する。 */
int limit_max_money(int balance) {
    return (balance <= MAX_BALANCE) ? 1 : 0;
}

/* 残高が購入金額以上かを判定する。 */
int can_afford(int balance, int price) {
    return (balance >= price) ? 1 : 0;
}

/* 単価と数量から合計購入金額を計算する。 */
int calc_total_price(int unit_price, int quantity) {
    return unit_price * quantity;
}

/* 残高と購入金額から釣銭額を計算する。 */
int calc_change_amount(int balance, int price) {
    return balance - price;
}

/* 現在の金種在庫で正確な釣銭を返せるか判定する。 */
int can_make_change(int change_amount) {
    int temp[DENOM_COUNT];
    if (change_amount < 0) {
        return 0;
    }
    if (change_amount == 0) {
        return 1;
    }
    return simulate_change_breakdown(change_amount, temp);
}

/* 釣銭額を金種別枚数に分解し、配列で返す。 */
int *make_change_breakdown(int change_amount) {
    static int change_counts[DENOM_COUNT];
    int i;
    for (i = 0; i < DENOM_COUNT; i++) {
        change_counts[i] = 0;
    }

    if (change_amount > 0) {
        simulate_change_breakdown(change_amount, change_counts);
    }
    return change_counts;
}

/* 釣銭払い出し後に自販機内金種枚数を減算する。 */
void apply_change_payout(int change_counts[]) {
    int i;
    for (i = 0; i < DENOM_COUNT; i++) {
        g_machine_money[i] -= change_counts[i];
        if (g_machine_money[i] < 0) {
            g_machine_money[i] = 0;
        }
    }
}

/* 1顧客分の取引状態を初期化する。 */
void start_transaction(void) {
    g_tx.balance = 0;
    g_tx.selected_product_id = 1;
    g_tx.quantity = 0;
    g_tx.total_price = 0;
    g_tx.active = 1;
}

/* 商品選択から在庫更新・釣銭処理までの購入フローを実行する。 */
int process_purchase(int product_id, int quantity, int balance) {
    Product *p;
    int total_price;
    SaleData sale;

    (void)quantity;

    if (product_id < 1 || product_id > MAX_PRODUCTS) {
        printf("商品番号が不正です。\n");
        return -1;
    }

    g_tx.selected_product_id = product_id;
    quantity = 1;

    p = &g_products[product_id - 1];
    total_price = calc_total_price(p->price, quantity);

    if (!can_afford(balance, total_price)) {
        printf("残高不足です。お金を追加投入してください。\n");
        log_error_csv(LOG_CSV_FILE, make_error_log("残高不足", "購入を中止しました。"));
        return -1;
    }

    if (reduce_stock(product_id, quantity) != 0) {
        printf("在庫更新に失敗しました。\n");
        log_error_csv(LOG_CSV_FILE, make_error_log("在庫更新失敗", "在庫の更新処理に失敗しました。"));
        return -1;
    }

    get_now_timestamp(sale.timestamp, sizeof(sale.timestamp));
    sale.product_id = p->id;
    snprintf(sale.product_name, sizeof(sale.product_name), "%s", p->name);
    sale.quantity = quantity;
    sale.unit_price = p->price;
    sale.total_price = total_price;
    sale.inserted_amount = balance;
    sale.change_amount = 0;

    g_last_sale = sale;
    g_has_last_sale = 1;

    log_sale_csv(LOG_CSV_FILE, sale);
    log_money_csv(LOG_CSV_FILE, g_machine_money);

    printf("商品を払い出します: %s x %d\n", p->name, quantity);
    printf("購入金額: %d 円\n", total_price);

    g_tx.balance = calc_change_amount(balance, total_price);
    g_tx.quantity = quantity;
    g_tx.total_price = total_price;

    finish_transaction();
    return 0;
}

/* 現在の取引をキャンセルし、残高を全額返金する。 */
void cancel_transaction(int balance) {
    printf("取引をキャンセルしました。返金額: %d 円\n", balance);
    log_operation_csv(LOG_CSV_FILE, make_operation_log("キャンセル", "利用者が取引をキャンセルしました。"));
    g_tx.balance = 0;
    reset_for_next_customer();
}

/* 取引完了時の通知やログ記録を行う。 */
void finish_transaction(void) {
    if (g_has_last_sale) {
        printf("購入処理が完了しました。\n");
        log_operation_csv(LOG_CSV_FILE,
                          make_operation_log("購入完了", "購入処理が正常終了しました。"));
    }
}

/* 次の顧客に備えて一時的な取引情報を初期化する。 */
void reset_for_next_customer(void) {
    g_tx.balance = 0;
    g_tx.selected_product_id = 1;
    g_tx.quantity = 0;
    g_tx.total_price = 0;
    g_tx.active = 0;
}

/* 売上ログをCSVへ追記する。 */
int log_sale_csv(const char *filename, SaleData sale) {
    FILE *fp;
    ensure_csv_headers(filename,
                       "type,timestamp,col1,col2,col3,col4,col5,col6,col7,col8\n");
    fp = fopen(filename, "a");
    if (fp == NULL) {
        return -1;
    }

    fprintf(fp,
            "売上,%s,%d,%s,%d,%d,%d,%d,%d\n",
            sale.timestamp,
            sale.product_id,
            sale.product_name,
            sale.quantity,
            sale.unit_price,
            sale.total_price,
            sale.inserted_amount,
            sale.change_amount);

    fclose(fp);
    return 0;
}

/* 自販機内の金種在庫ログをCSVへ追記する。 */
int log_money_csv(const char *filename, int money_counts[]) {
    FILE *fp;
    char ts[20];
    get_now_timestamp(ts, sizeof(ts));

    ensure_csv_headers(filename,
                       "type,timestamp,col1,col2,col3,col4,col5,col6,col7,col8\n");
    fp = fopen(filename, "a");
    if (fp == NULL) {
        return -1;
    }

    fprintf(fp,
            "金種残数,%s,%d,%d,%d,%d,%d,0,0\n",
            ts,
            money_counts[0],
            money_counts[1],
            money_counts[2],
            money_counts[3],
            money_counts[4]);

    fclose(fp);
    return 0;
}

/* 操作ログをCSVへ追記する。 */
int log_operation_csv(const char *filename, OperationLog log) {
    FILE *fp;
    ensure_csv_headers(filename,
                       "type,timestamp,col1,col2,col3,col4,col5,col6,col7,col8\n");
    fp = fopen(filename, "a");
    if (fp == NULL) {
        return -1;
    }

    fprintf(fp,
            "操作,%s,%s,%s,0,0,0,0,0\n",
            log.timestamp,
            log.action,
            log.detail);

    fclose(fp);
    return 0;
}

/* 異常系ログをCSVへ追記する。 */
int log_error_csv(const char *filename, ErrorLog log) {
    FILE *fp;
    ensure_csv_headers(filename,
                       "type,timestamp,col1,col2,col3,col4,col5,col6,col7,col8\n");
    fp = fopen(filename, "a");
    if (fp == NULL) {
        return -1;
    }

    fprintf(fp,
            "異常,%s,%s,%s,0,0,0,0,0\n",
            log.timestamp,
            log.error_code,
            log.detail);

    fclose(fp);
    return 0;
}

/* 故障予兆ログをCSVへ追記する。 */
int log_fault_prediction_csv(const char *filename, FaultPredictionLog log) {
    FILE *fp;
    ensure_csv_headers(filename,
                       "type,timestamp,col1,col2,col3,col4,col5,col6,col7,col8\n");
    fp = fopen(filename, "a");
    if (fp == NULL) {
        return -1;
    }

    fprintf(fp,
            "故障予兆,%s,%s,%d,%s,0,0,0,0\n",
            log.timestamp,
            log.fault_type,
            log.risk_score,
            log.detail);

    fclose(fp);
    return 0;
}

/* CSVファイルが新規作成時にヘッダ行を出力する。 */
int ensure_csv_headers(const char *filename, const char *headers) {
    FILE *fp;
    long size;

    fp = fopen(filename, "a+");
    if (fp == NULL) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    if (size == 0) {
        fputs(headers, fp);
    }

    fclose(fp);
    return 0;
}

/* 管理者向けの補充処理と金種状態表示を実行する。 */
void handle_admin_menu(void) {
    int done = 0;
    while (!done && !is_shutdown_requested()) {
        int menu;
        printf("\n=== 管理者メニュー ===\n");
        printf("1. 商品を補充\n");
        printf("2. 自販機内金種を表示\n");
        printf("0. 戻る\n");
        printf("00. 即時終了\n");
        printf("選択してください: ");
        menu = read_int_safe();

        if (is_shutdown_requested()) {
            done = 1;
            continue;
        }

        if (menu == 1) {
            int product_id;
            int qty;
            printf("補充する商品番号（00で終了）: ");
            product_id = read_int_safe();
            if (is_shutdown_requested()) {
                done = 1;
                continue;
            }
            printf("補充数量（00で終了）: ");
            qty = read_int_safe();
            if (is_shutdown_requested()) {
                done = 1;
                continue;
            }
            if (restock_product(product_id, qty) == 0) {
                printf("補充が完了しました。\n");
                log_operation_csv(LOG_CSV_FILE,
                                  make_operation_log("補充", "管理者が商品を補充しました。"));
            } else {
                printf("補充に失敗しました。\n");
                log_error_csv(LOG_CSV_FILE, make_error_log("補充失敗", "管理者補充処理が失敗しました。"));
            }
        } else if (menu == 2) {
            int i;
            printf("自販機内金種の状態:\n");
            for (i = 0; i < DENOM_COUNT; i++) {
                printf("%4d 円: %d 枚\n", g_denominations[i], g_machine_money[i]);
            }
            log_money_csv(LOG_CSV_FILE, g_machine_money);
        } else if (menu == 0) {
            done = 1;
        } else {
            printf("管理者メニューの選択が不正です。\n");
        }
    }
}

/* 複数顧客が連続利用できる自販機メインループを実行する。 */
void run_vending_machine(void) {
    int running = 1;

    while (running && !is_shutdown_requested()) {
        int choice;
        show_product_list();
        show_main_menu();
        choice = read_int_safe();

        if (is_shutdown_requested()) {
            running = 0;
            break;
        }

        if (choice == 1) {
            start_transaction();

            while (g_tx.active && !is_shutdown_requested()) {
                int input_menu;
                int add_amount = 0;
                int money_counts[DENOM_COUNT] = {0, 0, 0, 0, 0};

                show_product_list();
                printf("\n=== 金額投入画面 ===\n");
                show_current_balance(g_tx.balance);
                printf("1. 10円投入\n");
                printf("2. 50円投入\n");
                printf("3. 100円投入\n");
                printf("4. 500円投入\n");
                printf("5. 1000円投入\n");
                printf("6. 商品購入へ進む\n");
                printf("0. 取引キャンセル（返金して終了）\n");
                printf("00. 即時終了\n");
                printf("選択してください: ");
                input_menu = read_int_safe();

                if (is_shutdown_requested()) {
                    payout_and_end_transaction("利用者が00入力で終了しました。");
                    break;
                }

                if (input_menu == 0) {
                    payout_and_end_transaction("利用者が取引をキャンセルしました。");
                    break;
                }
                if (input_menu == 6) {
                    if (g_tx.balance <= 0) {
                        printf("先にお金を投入してください。\n");
                        continue;
                    }

                    while (g_tx.active && !is_shutdown_requested()) {
                        int product_id;
                        int min_price;

                        /* 要件に従い、商品一覧は状態に関係なく継続表示する。 */
                        show_product_list();
                        show_current_balance(g_tx.balance);

                        min_price = get_min_available_price();
                        if (min_price < 0) {
                            printf("購入可能な商品がありません。返金して終了します。\n");
                            payout_and_end_transaction("全商品売り切れのため取引終了。");
                            break;
                        }

                        if (g_tx.balance < min_price) {
                            printf("投入金額では購入可能な商品がありません。返金して終了します。\n");
                            payout_and_end_transaction("残高不足により取引終了。");
                            break;
                        }

                        printf("商品番号を入力してください（0:終了して返金 / 51:取引キャンセル / 00:即時終了）: ");
                        product_id = read_int_safe();

                        if (is_shutdown_requested()) {
                            payout_and_end_transaction("利用者が00入力で終了しました。");
                            break;
                        }

                        if (product_id == 0) {
                            payout_and_end_transaction("利用者が終了を選択しました。");
                            break;
                        }
                        if (product_id == 51) {
                            payout_and_end_transaction("利用者が取引キャンセルを選択しました。");
                            break;
                        }

                        if (product_id < 1 || product_id > MAX_PRODUCTS) {
                            printf("商品番号が範囲外です。\n");
                            continue;
                        }
                        if (is_sold_out(product_id)) {
                            printf("選択した商品は売り切れです。\n");
                            continue;
                        }

                        if (process_purchase(product_id, 1, g_tx.balance) != 0) {
                            printf("購入処理に失敗しました。\n");
                            continue;
                        }
                    }
                    break;
                }

                if (input_menu >= 1 && input_menu <= 5) {
                    add_amount = g_denominations[input_menu - 1];

                    if (!limit_max_money(g_tx.balance + add_amount)) {
                        printf("投入上限(%d円)を超えるため、この入力は受け付けません。\n", MAX_BALANCE);
                        log_error_csv(LOG_CSV_FILE,
                                      make_error_log("投入上限超過", "上限超過分の投入を拒否しました。"));
                        continue;
                    }

                    money_counts[input_menu - 1] = 1;
                    add_money_to_machine(money_counts);
                    g_tx.balance += add_amount;
                    printf("%d円を投入しました。\n", add_amount);
                    show_current_balance(g_tx.balance);
                    log_operation_csv(LOG_CSV_FILE,
                                      make_operation_log("金額投入", "利用者が金額を投入しました。"));
                    continue;
                }

                printf("選択が不正です。\n");
            }

            start_transaction();
        } else if (choice == 0) {
            running = 0;
        } else {
            printf("メニューの選択が不正です。\n");
            log_error_csv(LOG_CSV_FILE,
                          make_error_log("不正メニュー", "未定義のメニュー番号が選択されました。"));
        }
    }
}

/* 00入力による終了要求状態を返す。 */
static int is_shutdown_requested(void) {
    return g_shutdown_requested;
}

/* 在庫があり、かつ購入対象となる商品の最安価格を返す。 */
static int get_min_available_price(void) {
    int i;
    int min_price = -1;
    for (i = 0; i < MAX_PRODUCTS; i++) {
        if (g_products[i].stock > 0) {
            if (min_price < 0 || g_products[i].price < min_price) {
                min_price = g_products[i].price;
            }
        }
    }
    return min_price;
}

/* 返金可能なら釣銭を払い出し、取引を終了する。 */
static void payout_and_end_transaction(const char *reason) {
    int change_amount = g_tx.balance;
    int *change_counts;
    int i;

    if (reason != NULL) {
        log_operation_csv(LOG_CSV_FILE, make_operation_log("取引終了", reason));
    }

    if (change_amount <= 0) {
        printf("返金はありません。\n");
        reset_for_next_customer();
        return;
    }

    if (!can_make_change(change_amount)) {
        printf("正確な釣銭を返却できません。管理者へ連絡してください。\n");
        log_error_csv(LOG_CSV_FILE,
                      make_error_log("返金失敗", "釣銭不足で返金できませんでした。"));
        log_fault_prediction_csv(LOG_CSV_FILE,
                                 make_fault_log("釣銭リスク", 95, "返金用金種が不足しています。"));
        reset_for_next_customer();
        return;
    }

    change_counts = make_change_breakdown(change_amount);
    apply_change_payout(change_counts);
    printf("返金額: %d 円\n", change_amount);
    printf("返金内訳:　");
    for (i = DENOM_COUNT - 1; i >= 0; i--) {
        printf("%4d円: %d枚　", g_denominations[i], change_counts[i]);
    }
    log_money_csv(LOG_CSV_FILE, g_machine_money);
    reset_for_next_customer();
    printf("\n\n");
}

/* 固定幅セル内に文字列を中央寄せで表示する。 */
static void print_center_cell(const char *text, int width) {
    int len = utf8_display_width(text);
    int left;
    int right;
    char truncated[INPUT_BUF];

    if (len >= width) {
        utf8_truncate_display_width(text, width, truncated, sizeof(truncated));
        printf("%s", truncated);
        return;
    }

    left = (width - len) / 2;
    right = width - len - left;
    printf("%*s%s%*s", left, "", text, right, "");
}

/* UTF-8先頭バイトから1文字のバイト数を判定する。 */
static int utf8_char_bytes(unsigned char lead) {
    if ((lead & 0x80) == 0x00) {
        return 1;
    }
    if ((lead & 0xE0) == 0xC0) {
        return 2;
    }
    if ((lead & 0xF0) == 0xE0) {
        return 3;
    }
    if ((lead & 0xF8) == 0xF0) {
        return 4;
    }
    return 1;
}

/* UTF-8文字列の表示幅（半角1/全角2）を返す。 */
static int utf8_display_width(const char *s) {
    int width = 0;
    const unsigned char *p = (const unsigned char *)s;

    while (*p != '\0') {
        int bytes = utf8_char_bytes(*p);
        width += (*p < 0x80) ? 1 : 2;
        p += bytes;
    }

    return width;
}

/* UTF-8の継続バイトを壊さず、表示幅単位で切り詰める。 */
static void utf8_truncate_display_width(const char *src, int max_width, char *dst, size_t dst_size) {
    int width = 0;
    size_t out = 0;
    const unsigned char *p = (const unsigned char *)src;

    if (dst_size == 0) {
        return;
    }

    while (*p != '\0' && out + 1 < dst_size) {
        int char_width = (*p < 0x80) ? 1 : 2;
        size_t char_bytes = (size_t)utf8_char_bytes(*p);

        if (width + char_width > max_width) {
            break;
        }

        if (out + char_bytes >= dst_size) {
            break;
        }

        while (char_bytes-- > 0 && *p != '\0') {
            dst[out++] = (char)*p;
            p++;
        }

        width += char_width;
    }

    dst[out] = '\0';
}

/* エントリポイント。初期化→読込→メインループ→保存の順に実行する。 */
int main(void) {
    configure_console_utf8();
    init_products();

    if (load_products_from_csv(PRODUCT_CSV_FILE) != 0) {
        printf("商品CSVが見つからないか不正です。初期商品を読み込みます。\n");
        save_products_to_csv(PRODUCT_CSV_FILE);
    }

    ensure_csv_headers(LOG_CSV_FILE,
                       "type,timestamp,col1,col2,col3,col4,col5,col6,col7,col8\n");

    run_vending_machine();

    if (save_products_to_csv(PRODUCT_CSV_FILE) != 0) {
        printf("警告: 商品データの保存に失敗しました。\n");
    }

    printf("システムを終了しました。\n");
    return 0;
}
