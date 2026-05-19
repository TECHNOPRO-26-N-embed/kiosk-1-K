要件ファイルを確認して、実装対象の関数を漏れなく抽出し、10/50/100/500/1000円対応を反映した関数一覧をMarkdownで整理します。まず ai_requirements.md を読みます。

Read [](file:///c%3A/Users/User/Desktop/%E6%8A%80%E8%A1%93%E7%A0%94%E4%BF%AE%E7%94%A8%E3%83%95%E3%82%A1%E3%82%A4%E3%83%AB/1-k/kiosk-1-K/docs/ai_requirements.md)

# C言語 自販機システム：実装すべき関数一覧

以下は、要件に基づいた「まず動くプロトタイプ」を作るための関数一覧です。  
金額投入は 100円だけでなく、10円・50円・100円・500円・1000円に対応する前提で整理しています。

## 1. 商品・在庫管理

| 関数名 | 役割 |
|---|---|
| init_products | 商品50種類を初期化（商品名、価格、在庫上限50、現在在庫）。 |
| load_products_from_csv | 商品マスタをCSVから読み込む。起動時の在庫復元に使う。 |
| save_products_to_csv | 商品情報と在庫をCSVへ保存する。終了時や取引後に実行。 |
| is_sold_out | 指定商品の在庫が0か判定する。 |
| reduce_stock | 購入確定時に在庫を減算する。 |
| restock_product | 補充処理。上限50を超えないように在庫を加算する。 |

## 2. 画面表示・入力

| 関数名 | 役割 |
|---|---|
| show_main_menu | メインメニュー表示（購入、終了など）。 |
| show_product_list | 商品番号、商品名、価格、売り切れ状態を表示。 |
| show_current_balance | 現在投入金額を表示。 |
| read_int_safe | 投入する金額に対応する番号入力を受け取り反映 |
| input_product_no | 商品番号を受け取り、範囲チェックする。 |
| input_quantity | 購入数量を受け取り、1以上かつ在庫以内で検証する。 |

## 3. 金額投入・釣銭

| 関数名 | 役割 |
|---|---|
| input_money_counts | 10円・50円・100円・500円・1000円の投入枚数（または枚/枚/枚/枚/枚）を受け取る。 |
| calc_inserted_amount | 投入枚数から合計投入金額を計算する。 |
| add_money_to_machine | 自販機内の金種別保有枚数に投入分を加算する。 |
| limit_max_money | 投入可能な最大金額5000円を超えたら投入不可処理 |
| can_afford | 残高が購入金額以上か判定する。 |
| calc_total_price | 単価×数量で購入金額を算出する。 |
| calc_change_amount | 釣銭額（投入金額−購入金額）を計算する。 |
| can_make_change | 自販機内の金種在庫で釣銭を返せるか判定する。 |
| make_change_breakdown | 釣銭額を10/50/100/500単位の内訳に分解する。 |
| apply_change_payout | 釣銭払い出し後に自販機内金種枚数を減算する。 |

## 4. 取引処理

| 関数名 | 役割 |
|---|---|
| start_transaction | 1顧客分の取引状態を初期化する。 |
| process_purchase | 商品選択→数量→金額確認→在庫更新→釣銭処理までを実行する。 |
| cancel_transaction | キャンセル時に全額返金し、取引を終了する。 |
| finish_transaction | 取引完了処理（ログ保存、画面通知、状態クリア）。 |
| reset_for_next_customer | 次顧客のために残高や一時データをリセットする。 |

## 5. CSVログ・監視データ

| 関数名 | 役割 |
|---|---|
| log_sale_csv | 売上CSVへ記録（日時、商品、数量、単価、合計、投入、釣銭、残りの硬貨の枚数など）。 |
| log_money_csv | 10, 50, 100, 500, 1000円各種類の残り枚数を表示。　|
| log_operation_csv | 操作ログCSVへ記録（投入、選択、キャンセル、補充など）。 |
| log_error_csv | 異常系ログCSVへ記録（不正入力、在庫不足、釣銭不足など）。 |
| log_fault_prediction_csv | 故障予兆データCSVへ記録（商品詰まり、釣銭切れ傾向など）。 |
| ensure_csv_headers | CSV初回作成時にヘッダ行を出力する。 |

## 6. メイン制御

| 関数名 | 役割 |
|---|---|
| run_vending_machine | 複数顧客を連続処理するメインループ。 |
| handle_admin_menu | 補充や状態確認などの管理者向け処理。 |
| main | 初期化→メインループ開始→終了処理。 |

---

## 7. 最低限の実装順（おすすめ）

1. init_products  
2. show_product_list  
3. input_product_no / input_quantity  
4. input_money_counts（10/50/100/500/1000対応）  
5. calc_inserted_amount / calc_total_price / can_afford  
6. process_purchase / reduce_stock / calc_change_amount  
7. log_sale_csv  
8. run_vending_machine