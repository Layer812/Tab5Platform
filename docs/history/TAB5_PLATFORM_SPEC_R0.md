# Tab5 Platform R0 — 共通プラットフォーム設計仕様

**状態:** 設計・ソース構成調査完了（APIドラフト）。抽出・ビルド・Tab5実機動作は未実施。  
**作成日:** 2026-10-31  
**参照:** [X68KTab5](https://github.com/Layer812/X68KTab5)、R6実機確認済みソース `802a10bf915d221ae91c950029d5e812ad4d085a`。  
**目的:** 実績のあるX68KTab5のTab5ホスト処理を独立させ、ArcadeTab5（限定基板MAME）を実装する。将来的な他エミュレータ利用も妨げない。

## 1. 固定原則

1. **X68KTab5 Production R6を変更しない。** この文書とArcadeTab5の開発は別リポジトリ／別ビルドとする。X68KTab5への逆輸入は別途、互換性確認後に限る。
2. **旧ゲスト環境を共通化しない。** PX68K、X68000デバイス、Human68k、XDF/HDS、CRTC固有処理、スキャンコード変換、YM2151のX68000固有経路は移植対象外。
3. **再利用するのはTab5ホスト実装と実績ある規則。** LCD、タッチ、RGB565提示、DoubleFB所有権、音声PCM送出、USB HID、SD、メモリ割当、コア別タスク。
4. **ソース転用はファイル単位で権利確認する。** 共通APIを新設しても、移したソースの原著作権表示・ライセンス義務は残る。MAMEコード・ROMを混在させない。
5. **ROMはファームウェアに含めない。** 利用者が適法に準備したROMをSDから読む。配布対象にゲームROM、MAME BIOS、独自キー等を含めない。
6. **CPU1のゲスト時間をCPU0の描画やスピーカー待ちで不要に止めない。** キュー、所有権移転、通知を採用し、待ちが必要な境界を明文化する。
7. **非破壊のビルド・書込。** ArcadeTab5用アプリだけを独立ビルドし、X68KTab5のHumanXDF領域やパーティションテーブルを書き換えない。

## 2. 固定プラットフォーム・既知条件

| 項目 | 現行R6で確認した構成 | 新プラットフォーム方針 |
|---|---|---|
| ボード | M5Stack Tab5 / ESP32-P4 | 同一機種 |
| IDF | ESP-IDF 5.5.4 | 初期版固定 |
| CPU | HPコア2個、360MHzプロファイル | CPU1=エミュレーション、CPU0=ホスト処理を初期方針とする |
| LP | `tab5_lp_broker` はTouchJoy latest/control transport | 小さい通知経路に限定し、MAME固有処理を移さない |
| PSRAM | 有効（ESP32-P4 Hex PSRAM 200MHz設定） | メモリ確保ポリシーを一元化 |
| 画面 | 1280×720論理表示、現行ゲーム領域960×720（両脇160px） | 表示領域・縦画面回転・スケーリングをアプリ設定へ |
| LCD | M5GFX 0.2.35、ST7121/ST7123/ILI9881Cへの対応、DSI DoubleFB | 既存の検証済み表示初期化を基準にする |
| 音声 | M5Unified 0.2.28、ES8388 256fs、44.1kHz | PCMシンクを共通化。音源合成はMAME側 |
| USB | `usb_host_hid` 1.2.0、キーボード／マウス／Joypad | HID生イベント→汎用入力イベントを共通化 |
| SD | SDMMC / FATFS | 任意パスの列挙・読み込みに分離 |
| Flash | 16MiB設定、既存factory 0x10000/8MiB、HumanXDF 0x810000 | 開発・書込は既存領域の保護を前提に別アプリ運用 |

**注意:** 画面寸法と左右バーはX68KTab5固有レイアウト。MAME用の640×480、縦224×288等の表示寸法を固定しない。ESP32-P4でのMAME速度・使用RAMは未測定。

## 3. 論理アーキテクチャ

```text
ArcadeTab5 (メニュー / ゲーム・ROM選択)
    └─ arcade_adapter (フレーム / 音声 / 入力変換)
         └─ mame_subset (指定基板のCPU/映像/音声/ROMローダ)
                │
                ▼
         tab5_platform API  [エミュレータ非依存]
           ├─ video: 画面枠、RGB565、回転、無変更領域、present通知
           ├─ audio: 44.1kHz PCMリング、送出期限優先、ES8388
           ├─ input: USB HID + Touch → キー／ゲーム操作イベント
           ├─ storage: SDマウント・ファイル列挙・ROM読込
           ├─ memory: Internal SRAM / PSRAMの確保規則
           └─ system: タスク配置・時刻・LP optional
                │
                ▼
         ESP-IDF 5.5.4 / M5Unified / M5GFX
                │
                ▼
               Tab5
```

### コア配置の初期設計

- **HP CPU1:** MAME基板のCPU実行、割り込み、タイマ、描画情報確定、入力取り込み。ホストUIの描画待ちはしない。
- **HP CPU0:** LCD・タッチ・合成、PCM出力、USB HID処理、SD、ランチャー。音声供給の締切を描画より優先する。
- **LP Core:** 必要性を示せた軽量通信に限る。MAME初期移植の必須依存にしない。

### 画面／音声の所有権

- ゲーム側は完成したフレームを**不変のスロット**として発行。LCD側のpresent完了まで同一メモリを再書き込みしない。
- DoubleFBの物理バッファ所有権と、ゲームフレームの完成状態を分離する。
- 初期版では **RGB565 / 画面回転（0°/90°/180°/270°）/ 黒帯**を対象とする。差分更新は明示的なdirty領域がある場合のみ利用。
- PCMはゲーム側で混合／リサンプル後、16-bit stereo・44.1kHzで供給。CPU0の音声フィーダは、再生可能データを先にSpeakerへ送るR6の優先処理を踏襲する。
- ロック待ちによる無期限停止を禁止し、キュー満杯・ドロップ・アンダーランを明示的に記録する（発売ビルドで常時大量ログは出さない）。

## 4. ソース抽出マップ（X68KTab5 mainの現物に基づく）

| 既存ファイル・機構 | 分類 | ArcadeTab5での扱い |
|---|---|---|
| `src/tab5_video.cpp` | 共通候補＋X68000/UI結合 | DSI、DoubleFB、表示所有権を抽出。X68000ホットキー・メディアUI・画面幅前提を分離 |
| `src/tab5_screen_manager.c` | 設計再利用候補 | 不変版／トークンの設計を参考に、汎用frame APIを新設。PX68K向けline latchを流用しない |
| `src/tab5_ppa.c` | アクセラレーション候補 | 実装を確認してPIE/scalarの有効部分のみ転用。PPA自動高速化は仮定しない |
| `src/tab5_audio.cpp` | 共通候補＋X68000強結合 | PCMリング、M5Unified Speaker、256fs関連のホスト部を分離。MSM6258/WinX68k/YM2151ロジックは除外 |
| `src/tab5_usb_keyboard.c` | 共通候補＋キー変換 | USB HID列挙・イベント取得を共通化。X68000マッピングは除外 |
| `src/tab5_guest_input.c` | X68000結合 | 生HIDイベントを基板別操作へ変換する別adapterで置換 |
| `src/tab5_sd.c` | SD＋XDF結合 | SDマウント等の共通部のみ抽出。XDF検索を移植しない |
| `src/tab5_media_ui.cpp`, `tab5_launcher.cpp` | X68000 UI結合 | ArcadeTab5独自のROMブラウザで置換。大きめのフォントで実装 |
| `src/tab5_lp_broker.c`, `src/ulp/lp_broker.c` | 条件付き候補 | TouchJoy通知だけを必要時に抽出 |
| `src/tab5_cpu_clock.c` | 共通候補 | 安定した360MHz構成を維持し、オーバークロック経路は作らない |
| `src/tab5_compose.c` | X68000レンダラ結合 | X68000 BG/GVRAM/TEXTの経路は転用しない |
| `src/tab5_guest_bus.c`, `src/tab5_disk_control.c` | X68000依存 | 共通基盤へ含めない |
| `components/px68k/` | X68000エミュレーション | **依存させない**（共通ライブラリに含めない） |
| `patch_m5gfx_x68ktab.py`, `patch_m5unified_*` | 固定版向けパッチ | 内容・対象SHAと適用条件を保持して、ArcadeTab5で必要な最小処理として独立管理 |
| `src/CMakeLists.txt` | PX68Kへの強い依存あり | 新たに `tab5_platform` を独立ESP-IDFコンポーネントとして定義 |
| ルート `platformio.ini` | Cardputer向け旧内容 | **移植しない。** ArcadeTab5はNative ESP-IDFを正規のビルド経路にする |

## 5. 共通APIの責務（ドラフト）

独立したヘッダー案は `include/tab5_platform.h` を参照。

- `t5p_init()` / `t5p_shutdown()` はTab5の表示・音声・入力・SDのライフサイクルを所有する。
- `t5p_video_acquire()` / `t5p_video_present()` はRGB565フレームスロットの所有権を移動する。成功後は旧ポインタを書き換えない。
- `t5p_audio_submit()` はMAMEで混合済みのinterleaved S16 stereoを供給し、**受理したフレーム数**を返す。実装で音声データの格納を保証するまで受理を返さない。
- `t5p_input_poll()` はソースに依存しない入力イベントを返す。X68000のスキャンコードはここへ出さない。
- `t5p_storage_mount()` / `t5p_storage_enumerate()` はゲーム種類を理解せず、汎用SD処理を提供する。
- `t5p_time_us()` は単調増加のホスト時刻。MAMEの仮想クロックと混同しない。

**APIは設計契約であり、ここに書いた関数は現時点では実装済みではない。** 実装前にタスク間排他、フレームスロット数、音声リング長、ROMの同時アクセスの境界を定める。

## 6. ファイル配置案

```text
ArcadeTab5/
├─ CMakeLists.txt
├─ sdkconfig.defaults               # P4 360MHz / PSRAM / IDF 5.5.4
├─ partitions.csv                  # ROMやHumanXDFを含めない独立設定
├─ main/                           # arcade app_main / menu
├─ components/
│  ├─ tab5_platform/
│  │  ├─ include/tab5_platform.h   # 機種共通API
│  │  └─ src/                      # Display / Audio / Input / SD / System
│  ├─ arcade_adapter/              # MAMEの出力→Tab5共通API
│  └─ mame_subset/                 # 選択した基板とCPU/音源のみ
├─ docs/
│  ├─ TAB5_PLATFORM_SPEC_R0.md
│  └─ ARCADETAB5_PORT_PLAN_R0.md
└─ THIRD_PARTY_NOTICES.md
```

ROM配置（案）: `/sdcard/arcade/roms/<rom-set>.zip`。このパスは設計値で、既存ソフトの格納位置を変更させるものではない。

## 7. 実装の順序と合格基準

1. **Platform R1:** PX68Kへの依存がゼロのESP-IDFコンポーネントとして、LCD・タッチ・USB・SD・PCM出力を構築。X68KTab5 R6のコード変更はゼロ。
2. **Host実機確認:** 大きな読みやすいメニュー、SDファイル列挙、入力操作、RGB565表示、44.1kHz PCM再生までをArcadeTab5の単一アプリで確認。診断専用ファームを増やさず、完成機能から順に確認。
3. **MAME subset R1:** 最初の8bit基板（Pac-Man系を暫定第一候補）を接続。合法的に用意したROMを外部SDから読み、映像・音・操作を実機で確認。
4. **MAME subset R2:** 縦横表示とフレームペーシングを整える。実機のフレーム時間・音声アンダーラン・PSRAM使用量で採用範囲を決定。
5. **限定基板の追加:** CPU/音源の既存モジュール共有を優先。CPS-1など68000系は別フェーズとし、性能を実測して判断。
6. **公開:** ソース・ビルド手順・第三者ライセンス・非ROM配布を明記し、M5Burner配布BINの再現性と実機動作を確認。

## 8. ライセンス・公開条件の判断点

- 旧MAME 0.37b5は**旧MAME非商用ライセンス**に従う。無償の公開だから何でも自由ではない。原文と選択ドライバに付属する第三者ライセンスを採用前に精査する。
- 将来、商用利用やライセンスの柔軟性を重視する場合は、MAME 0.172以降からBSD-3-Clause対象部分を個別に選別する別方式を検討する。0.172以降のMAME全体とBSD対象ファイルは同じ意味ではない。
- X68KTab5現行リポジトリには、個別ライセンス文書はあるが、トップレベル`LICENSE`は公開ルート一覧で確認できなかった。ArcadeTab5の独自コードのライセンスを**明示的に新設**し、転用元の各通知を保存する。
- 商用ROM、BIOS、商標素材は含めない。MAMEの名称・ロゴは公式承認と誤解されないよう表示する。
- ESP-IDF、M5Unified、M5GFX、USB Host HID、移植対象ソースの個別ライセンスもバイナリ配布対象に応じて確認する。

## 9. 未確定事項（実装時に決める）

- ROMセット互換性（MAME 0.37b5と選択ドライバの組み合わせ）、ZIPサポート範囲。
- 音声リング容量・フレームスロット数・性能予算は実機結果によって確定。
- アプリのパーティションレイアウトは、ROMを内蔵しない構成で、**既存機器の他領域を壊さない書込方式**とともに確定。
- LCD縦横のスケーラ・回転処理の最適化は、ゲーム本体の動作確認後に限定して実施。

## 10. 参照元（2026-10-31にGitHubで直接確認）

- https://github.com/Layer812/X68KTab5/blob/main/README.md
- https://github.com/Layer812/X68KTab5/blob/main/RELEASE_NOTES_2026-10-28.md
- https://github.com/Layer812/X68KTab5/blob/main/src/CMakeLists.txt
- https://github.com/Layer812/X68KTab5/tree/main/src
- https://github.com/Layer812/X68KTab5/blob/main/partitions.csv
- https://github.com/Layer812/X68KTab5/blob/main/dependencies.lock
- https://github.com/Layer812/X68KTab5/blob/main/docs/M5GFX_0235_FEATURE_ADOPTION_AUDIT.md
- https://github.com/Layer812/X68KTab5/blob/main/THIRD_PARTY_NOTICES.md
