# 25Ja Tool

UV-K1 / UV-K5(K6) V3 用 25Ja（にこじゃ）F/Wの書込ツール。PC の Chrome / Edge（Web Serial）で動く 1 ページのツール。

## 機能
0. 初めての手順（校正バックアップ → F/W → 初期化 → 書体データ → 校正復元。番号で各欄へ移動し、結果を ✔ / ✖ で表示）
1. 校正データのバックアップ（512 B）
2. F/Wの書込（F/W 書込モード：PTT を押したまま電源 ON）。25Ja を初めて入れた直後は送信できない旨を欄に表示
3. 書体データ（東雲 12 ドット, 6,879 字, v5）の書込。25Ja 同士はセクタ単位の高速書込（0x0523）、古いF/Wは従来方式。最後に全体を照合
4. 校正データの復元
5. 設定領域の一括バックアップ・復元（SPI 0xA000–0xA21F, 544 B の生バイト。NeonPlugJA が扱わない領域も含む。旧 512 B 形式も復元可）
6. 起動画像（128×64、見出し `25Ja`）の書込・無効化

そのほか: FM 局名の編集は別ページ（`fm.html`）。ログのコピー（版・日時付き）・消去、「ファイルを選ぶ」ボタン（選んだ名前と検査結果を欄に表示）、接続時に本体の 25Ja 目印（挨拶応答 Version[13..15] = 0x25 'J' 機能ビット, bit0 = CH 名 31 B）をログに表示。

誤操作防止: F/W欄は先頭のベクタ表（スタック値・開始番地）とサイズ、書体データ欄はサイズ・先頭の値・版数を確かめ、合わないファイルは受け付けない。F/W 書込モード/通常起動の取り違えも検出して止める。接続直後の受信エラー（Buffer overrun）が出ても受信を続ける（0.1.19 から）。

## 使い方
- 利用者: ZIP を展開し `index.html` をダブルクリック（Chrome / Edge）。同梱のF/W・書体データはボタンを押すとファイル選択が出るので、`firmware/` `font/` のファイルを選ぶ。
- 開発者: `cd web && python3 -m http.server 8000` → `http://localhost:8000` で開くと同梱ファイルを自動で読む（https で公開した場合も同じ）。
- 直開き（file://）でも、Web Serial の接続・読み書きができます（Chrome / Edge、Windows）。

## ファイル
- index.html — ツール本体（外部ライブラリなし, Ver 0.1.22）
- manual.html — 説明書（初回の書込・更新・キー・スキャン・スペクトラム・FM 放送の操作・FM 局名・困ったとき）
- fm.html — 25Ja FM局名ツール（Ver 0.1.11、単一 HTML）。FM ラジオの局名表（最大 127 局, SPI 0x012000）の読出・書込・消去、本体の FM プリセット 48CH との取り込み・書込、CSV の入出力、内蔵の局名データベース（総務省、令和7年4月1日現在、民放のみ）。25Ja Tool とは別のページ
- firmware/25Ja_v0.5.1.bin — 同梱F/W
- font/25Ja_font_v5.bin — 同梱書体データ
- test/sim.js — 模擬無線機を使った動作試験（Node.js + jsdom）。`CASES='^3' node sim.js` で一部だけ実行（全 22 件）

## 関連
PC の CPS「NeonPlugJA」（https://github.com/ja2izu/NeonPlugJA。この配布物には含まれません）は、infamy 氏の NeonPlug（MIT License、https://github.com/infamy/NeonPlug）をもとにしたフォークです。

## ライセンス
Apache License 2.0（LICENSE / NOTICE）。© 2026 JA2IZU。Dondji 公式のものではありません。
