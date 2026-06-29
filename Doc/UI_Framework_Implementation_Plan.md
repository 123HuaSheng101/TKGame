# TKGame 基本游戏玩法 UI 框架 — 实现方案

> 生成日期：2026-06-29
> 基于现有代码：28 个 C++ 源文件 + MVC_UI_Design.md + 全链路追踪文档

---

## 一、现状与缺口总结

### 已有（C++ 侧 ✅）

```
TKGameUIData.h          → FTKGameUIData / FTKPlayerUIData / FTKResponseUIData  已定义
TKGameHUD.h/.cpp         → MVC Controller，每帧推送数据，响应面板生命周期管理       已实现
TKGameHudWidget.h/.cpp   → View 接口层，BlueprintImplementableEvent 事件          已实现
TKCardSlotWidget.h/.cpp  → 单张手牌 View 接口                                    已实现
TKGameTypes.h            → FTKCardInstance（轻量USTRUCT，可网络复制）              已定义但未使用
```

### 缺失（需要新增/修改 🔴）

| 缺口 | 说明 |
|---|---|
| **卡牌显示数据** | 无 `FTKCardUIData` 结构体，蓝图无法直接读取卡牌花色/点数/名称（P4：UObject不复制） |
| **手牌数据推送** | `GetMyHandCards()` 返回 `UTKCardBase*` 数组，客户端读到的是默认值 |
| **装备区/判定区** | `FTKPlayerUIData` 没有装备和判定区信息 |
| **选区状态管理** | HUD 中没有"当前选中了哪张牌"、"选目标模式"等交互状态 |
| **阶段感知** | HUD 不知道自己是否在出牌阶段、是否轮到自己 |
| **蓝图控件不完整** | 现有 `WBP_GameHud`/`W_CardItem` 等是空壳，未绑定数据 |

---

## 二、UI 面板布局设计

```
┌──────────────────────────────────────────────────────────────┐
│  TOP BAR（顶部信息栏）                                         │
│  回合:3  当前:玩家A  阶段:出牌阶段  牌堆:24张  弃牌堆:16张     │
├──────────────────────────────────────────────────────────────┤
│                    ┌──────────┐                               │
│    ┌─────────┐     │  玩家B    │     ┌─────────┐              │
│    │  玩家C   │     │ 主公 ♛   │     │  玩家D   │              │
│    │ 身份???  │  血条│ 体力3/4  │血条  │ 身份???  │              │
│    │ 体力3/3  │     │ 手牌x4   │     │ 体力4/4  │              │
│    │ 手牌x5   │     └──────────┘     │ 手牌x2   │              │
│    └─────────┘         ▲              └─────────┘              │
│                   当前回合                                        │
│                                                               │
├──────────────────────────────────────────────────────────────┤
│  MY EQUIPMENT（装备区）      MY JUDGEMENT（判定区）             │
│  [武器] [防具] [+1马]       [乐不思蜀] [闪电]                  │
│  [-1马] [宝物]                                               │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│  MY HAND CARDS（手牌区）                                       │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐                        │
│  │杀 ♠A │ │闪 ♥2 │ │桃 ♦3 │ │酒 ♣K │  ...                   │
│  └──────┘ └──────┘ └──────┘ └──────┘                        │
│                                                               │
├──────────────────────────────────────────────────────────────┤
│  ACTION BAR（操作栏）                                          │
│  [使用]  [结束回合]                                             │
│                                                               │
│  RESPONSE PANEL（响应面板，平时隐藏）                           │
│  ⚠ 你需要出一张【闪】来响应【杀】    [10s]                     │
│  ┌──────┐ ┌──────┐    [出牌]  [放弃]                         │
│  │闪 ♥2 │ │闪 ♦5 │                                           │
│  └──────┘ └──────┘                                           │
└──────────────────────────────────────────────────────────────┘
```

---

## 三、C++ 代码修改清单

### 步骤 1：扩展 `TKGameUIData.h` — 新增卡牌 UI 数据协议

#### 1.1 新增 `FTKCardUIData`

```cpp
// ===== 新增：卡牌 UI 显示数据（解决 P4 UObject 不复制问题） =====

/**
 * 单张卡牌的 UI 显示数据（轻量纯数据结构）
 * 由 ATKGameHUD 从 UTKCardBase* 提取，供蓝图 Widget 直接读取
 */
USTRUCT(BlueprintType)
struct FTKCardUIData
{
    GENERATED_BODY()

    /** 运行时实例 ID（来自 FTKCardInstance） */
    UPROPERTY(BlueprintReadOnly)
    int32 InstanceId = INDEX_NONE;

    /** 卡牌模板 ID */
    UPROPERTY(BlueprintReadOnly)
    FName CardDefId;

    /** 显示名称 */
    UPROPERTY(BlueprintReadOnly)
    FText CardName;

    /** 卡牌类型 */
    UPROPERTY(BlueprintReadOnly)
    ETKCardType CardType = ETKCardType::Basic;

    /** 花色 */
    UPROPERTY(BlueprintReadOnly)
    ETKCardSuit Suit = ETKCardSuit::NoSuit;

    /** 点数 (1-13, A-K) */
    UPROPERTY(BlueprintReadOnly)
    int32 Rank = 0;

    /** 花色颜色（红桃♥/方块♦ = true, 黑桃♠/梅花♣ = false） */
    UPROPERTY(BlueprintReadOnly)
    bool bIsRed = false;

    /** 所在牌区 */
    UPROPERTY(BlueprintReadOnly)
    ETKCardZone ZoneType = ETKCardZone::Hand;

    /** 花色 Unicode 字符 */
    UPROPERTY(BlueprintReadOnly)
    FString SuitSymbol;

    /** 点数显示字符串（A, 2-10, J, Q, K） */
    UPROPERTY(BlueprintReadOnly)
    FString RankText;

    /** 工具方法：从 UTKCardBase 生成 FTKCardUIData */
    static FTKCardUIData FromCard(class UTKCardBase* Card);
};
```

#### 1.2 新增 `FTKActionUIData`

```cpp
/**
 * 当前可执行操作状态
 */
USTRUCT(BlueprintType)
struct FTKActionUIData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bIsMyTurn = false;

    UPROPERTY(BlueprintReadOnly)
    bool bIsPlayPhase = false;

    UPROPERTY(BlueprintReadOnly)
    bool bIsDiscardPhase = false;

    UPROPERTY(BlueprintReadOnly)
    bool bCanUseSlash = false;

    UPROPERTY(BlueprintReadOnly)
    bool bCanEndTurn = false;

    UPROPERTY(BlueprintReadOnly)
    int32 SelectedCardIndex = -1;
};
```

#### 1.3 扩展 `FTKPlayerUIData`

在现有 `FTKPlayerUIData` 中追加以下字段：

```cpp
    /** 装备区卡牌 */
    UPROPERTY(BlueprintReadOnly)
    TArray<FTKCardUIData> EquipmentCards;

    /** 判定区卡牌 */
    UPROPERTY(BlueprintReadOnly)
    TArray<FTKCardUIData> JudgementCards;
```

---

### 步骤 2：扩展 `TKGameHUD.h` — Controller 层增强

在现有类体中追加以下声明：

```cpp
    // ===== 新增：选区管理 =====

    /** 选中手牌索引 */
    UFUNCTION(BlueprintCallable)
    void SelectCard(int32 CardIndex);

    UFUNCTION(BlueprintCallable)
    void ClearSelection();

    UPROPERTY(BlueprintReadOnly)
    int32 SelectedCardIndex = -1;

    // ===== 新增：手牌数据推送 =====

    UFUNCTION(BlueprintCallable, BlueprintPure)
    TArray<FTKCardUIData> GetMyHandCardUIData() const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    FTKActionUIData GetActionUIData() const;

    // ===== 新增：状态查询 =====

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsMyTurn() const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    ATKPlayerStateBase* GetMyPlayerState() const;

protected:
    /** 手牌数据是否有变化 */
    bool bHandCardsDirty = true;

    /** 上次 Tick 时的回合号（用于检测回合切换） */
    int32 LastTurnNumber = 0;

    /** 上次 Tick 时的阶段（用于检测阶段切换） */
    ETKTurnPhase LastPhase = ETKTurnPhase::Prepare;

    /** 上次 Tick 时的游戏结果 */
    ETKGameResult LastGameResult = ETKGameResult::None;
```

---

### 步骤 3：修改 `TKGameHUD.cpp` — Controller 实现

#### 3.1 `FTKCardUIData::FromCard()` 工具方法

```cpp
FTKCardUIData FTKCardUIData::FromCard(UTKCardBase* Card)
{
    FTKCardUIData Data;
    if (!Card) return Data;

    Data.CardDefId = Card->CardDefId;
    Data.CardName  = Card->CardName;
    Data.CardType  = Card->CardType;
    Data.Suit      = Card->Suit;
    Data.Rank      = Card->Rank;
    Data.ZoneType  = Card->ZoneType;

    // 花色颜色
    Data.bIsRed = (Card->Suit == ETKCardSuit::Heart || Card->Suit == ETKCardSuit::Diamond);

    // Unicode 花色符号
    static const TCHAR* SuitSymbols[] = { TEXT("♠"), TEXT("♥"), TEXT("♣"), TEXT("♦"), TEXT("") };
    uint8 SuitIdx = FMath::Min((uint8)Card->Suit, (uint8)4);
    Data.SuitSymbol = SuitSymbols[SuitIdx];

    // 点数文本
    if (Card->Rank == 1)       Data.RankText = TEXT("A");
    else if (Card->Rank <= 10) Data.RankText = FString::FromInt(Card->Rank);
    else if (Card->Rank == 11) Data.RankText = TEXT("J");
    else if (Card->Rank == 12) Data.RankText = TEXT("Q");
    else if (Card->Rank == 13) Data.RankText = TEXT("K");
    else                       Data.RankText = TEXT("?");

    return Data;
}
```

#### 3.2 改造后的 `Tick()` — 事件驱动 + 脏标记

```cpp
void ATKGameHUD::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!MainHud) return;

    // 获取当前游戏数据
    ATKGameStateBase* TKGS = GetWorld() ? Cast<ATKGameStateBase>(GetWorld()->GetGameState()) : nullptr;
    UTKTurnComponentBase* TurnComp = TKGS ? TKGS->GetTurnComponent() : nullptr;

    int32 CurrentTurn = TurnComp ? TurnComp->GetCurrentTurnNumber() : 0;
    ETKTurnPhase CurrentPhase = TurnComp ? TurnComp->GetCurrentPhase() : ETKTurnPhase::Prepare;
    ETKGameResult CurrentResult = TKGS ? TKGS->GameResult.WinnerFaction : ETKGameResult::None;

    // 1. 回合变更 → 全量刷新
    if (CurrentTurn != LastTurnNumber)
    {
        MainHud->RefreshDisplay(GetGameUIData(), GetAllPlayerUIData());
        bHandCardsDirty = true;
        LastTurnNumber = CurrentTurn;
    }

    // 2. 阶段变更 → 刷新操作状态
    if (CurrentPhase != LastPhase)
    {
        MainHud->OnActionStateUpdated(GetActionUIData());
        LastPhase = CurrentPhase;
    }

    // 3. 游戏结束 → 结算界面
    if (CurrentResult != LastGameResult && CurrentResult != ETKGameResult::None)
    {
        MainHud->OnGameResultShown(GetGameUIData());
        LastGameResult = CurrentResult;
    }

    // 4. 手牌变更 → 仅刷新手牌区（由 MarkHandCardsDirty() 触发）
    if (bHandCardsDirty)
    {
        MainHud->OnMyHandCardsUpdated(GetMyHandCardUIData());
        bHandCardsDirty = false;
    }

    // 5. 响应倒计时
    if (ResponseTimeRemaining > 0.0f)
    {
        ResponseTimeRemaining = FMath::Max(0.0f, ResponseTimeRemaining - DeltaSeconds);
        FTKResponseUIData RespData;
        RespData.TimeRemaining = ResponseTimeRemaining;
        RespData.bIsWaiting = true;
        MainHud->SetResponseUIData(RespData);
    }
}
```

#### 3.3 新增方法实现

```cpp
TArray<FTKCardUIData> ATKGameHUD::GetMyHandCardUIData() const
{
    TArray<FTKCardUIData> Result;
    TArray<UTKCardBase*> Hand = GetMyHandCards();
    Result.Reserve(Hand.Num());
    for (UTKCardBase* Card : Hand)
    {
        Result.Add(FTKCardUIData::FromCard(Card));
    }
    return Result;
}

FTKActionUIData ATKGameHUD::GetActionUIData() const
{
    FTKActionUIData Data;
    ATKGameStateBase* TKGS = GetWorld() ? Cast<ATKGameStateBase>(GetWorld()->GetGameState()) : nullptr;
    UTKTurnComponentBase* TurnComp = TKGS ? TKGS->GetTurnComponent() : nullptr;
    if (!TurnComp) return Data;

    ATKPlayerStateBase* MyPS = GetMyPlayerState();
    APlayerState* CurrentPlayer = TurnComp->GetCurrentPlayer();

    Data.bIsMyTurn      = (MyPS == CurrentPlayer);
    Data.bIsPlayPhase   = (TurnComp->GetCurrentPhase() == ETKTurnPhase::Play);
    Data.bIsDiscardPhase= (TurnComp->GetCurrentPhase() == ETKTurnPhase::Discard);
    Data.bCanUseSlash   = Data.bIsMyTurn && Data.bIsPlayPhase && TurnComp->CanUseSlash();
    Data.bCanEndTurn    = Data.bIsMyTurn;
    Data.SelectedCardIndex = SelectedCardIndex;

    return Data;
}

bool ATKGameHUD::IsMyTurn() const
{
    ATKGameStateBase* TKGS = GetWorld() ? Cast<ATKGameStateBase>(GetWorld()->GetGameState()) : nullptr;
    UTKTurnComponentBase* TurnComp = TKGS ? TKGS->GetTurnComponent() : nullptr;
    if (!TurnComp) return false;

    ATKPlayerStateBase* MyPS = GetMyPlayerState();
    return MyPS == TurnComp->GetCurrentPlayer();
}

ATKPlayerStateBase* ATKGameHUD::GetMyPlayerState() const
{
    ATKPlayerControllerBase* PC = Cast<ATKPlayerControllerBase>(GetOwningPlayerController());
    return PC ? Cast<ATKPlayerStateBase>(PC->PlayerState) : nullptr;
}

void ATKGameHUD::SelectCard(int32 CardIndex)
{
    SelectedCardIndex = CardIndex;
    // 通知 Widget 更新选中状态
    if (MainHud)
    {
        TArray<FTKCardUIData> HandData = GetMyHandCardUIData();
        if (CardIndex >= 0 && CardIndex < HandData.Num())
        {
            MainHud->OnCardSelectionChanged(CardIndex, HandData[CardIndex]);
        }
        else
        {
            MainHud->OnCardSelectionChanged(-1, FTKCardUIData());
        }
    }
}

void ATKGameHUD::ClearSelection()
{
    SelectCard(-1);
}

void ATKGameHUD::MarkHandCardsDirty()
{
    bHandCardsDirty = true;
}
```

---

### 步骤 4：修改 `TKGameHudWidget.h` — View 接口增强

追加以下 `BlueprintImplementableEvent`：

```cpp
    /** 蓝图事件：手牌数据更新 */
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void OnMyHandCardsUpdated(const TArray<FTKCardUIData>& Cards);

    /** 蓝图事件：操作状态变更 */
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void OnActionStateUpdated(const FTKActionUIData& ActionData);

    /** 蓝图事件：选牌变更 */
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void OnCardSelectionChanged(int32 SelectedIndex, const FTKCardUIData& CardData);

    /** 蓝图事件：游戏结束 */
    UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
    void OnGameResultShown(const FTKGameUIData& GameData);
```

---

### 步骤 5：改造数据流 — 从"每帧推送"改为"事件驱动+脏标记"

```
修改前（每帧全量推送，低效）：
  Tick() → RefreshDisplay(GameData, PlayerData) → 60fps 全刷新

修改后（事件驱动，高效）：
  Tick() → 仅检查脏标记
    ├── bHandCardsDirty → OnMyHandCardsUpdated(HandCards)     // 手牌变化时
    ├── bTurnChanged    → RefreshDisplay(GameData, PlayerData) // 回合切换时
    ├── bPhaseChanged   → OnActionStateUpdated(ActionData)     // 阶段切换时
    └── bResultChanged  → OnGameResultShown(GameData)          // 游戏结束时
```

---

## 四、蓝图 View 层搭建方案

### 蓝图文件规划

| 蓝图 | 继承 | 职责 |
|---|---|---|
| `WBP_GameHud` | `UTKGameHudWidget` | **主容器**：布局所有子面板 |
| `WBP_TopBar` | `UUserWidget` | 顶部信息栏（回合/阶段/牌堆数） |
| `WBP_PlayerSeat` | `UUserWidget` | 单个玩家座位（头像/身份/血条/手牌数） |
| `WBP_SeatLayout` | `UUserWidget` | 座位布局容器（管理多个 PlayerSeat） |
| `WBP_CardItem` | `UTKCardSlotWidget` | **手牌卡牌**（花色/点数/名称 + 高亮） |
| `WBP_HandArea` | `UUserWidget` | **手牌区**（水平排列，管理 CardItem 列表） |
| `WBP_EquipSlot` | `UUserWidget` | 装备槽单格 |
| `WBP_EquipArea` | `UUserWidget` | 装备区（5 个槽位） |
| `WBP_ActionBar` | `UUserWidget` | 操作按钮（使用/结束回合） |
| `WBP_ResponsePanel` | `UUserWidget` | 响应面板（提示/倒计时/可用牌/出牌/放弃） |

---

### 蓝图搭建详细步骤

#### Step 1: `WBP_CardItem`（手牌卡牌 Widget）

```
继承: UTKCardSlotWidget

控件层级:
  Canvas Panel
    ├── Image_CardBg         （卡牌底图）
    ├── Image_SuitIcon       （花色图标：♠♣♥♦）
    ├── TextBlock_CardName   （卡牌名：杀/闪/桃/酒）
    ├── TextBlock_Rank       （点数：A/2/3...K）
    ├── TextBlock_Suit       （花色文字：♠♣♥♦）
    └── Image_SelectionBorder （选中高亮边框，默认隐藏）

事件绑定（蓝图）:
  Event OnCardDataUpdated(Card)
    → 读取 Card.CardName / Card.Suit / Card.Rank
    → 设置 TextBlock 文本
    → 根据 Suit 切换 Image_SuitIcon 颜色（♥♦=红, ♠♣=黑）

  Event OnCardClickedBP()
    → 调用 OwningHUD → SelectCard(CardIndex)

绑定变量:
  SelectedCardIndex  → 更新 Image_SelectionBorder 显隐
```

#### Step 2: `WBP_HandArea`（手牌区容器）

```
继承: UUserWidget

控件层级:
  Panel → HorizontalBox / WrapBox
    动态生成 WBP_CardItem 实例

事件:
  OnMyHandCardsUpdated(Cards数组)
    → ClearChildren()
    → ForEach Card in Cards:
        CreateWidget(WBP_CardItem)
        → SetCardData(Card, Index)
        → AddToHorizontalBox
```

#### Step 3: `WBP_PlayerSeat`（单个玩家座位）

```
继承: UUserWidget

控件层级:
  VerticalBox
    ├── Image_Avatar          （头像/武将图）
    ├── TextBlock_PlayerName  （玩家名）
    ├── TextBlock_Identity    （身份：主公/忠臣/反贼/内奸/???）
    ├── HorizontalBox_HP      （体力条：♥ x N）
    │    └── 动态 Image 填充当前体力
    ├── HorizontalBox_Equip   （装备图标）
    │    └── 5 个 WBP_EquipSlot
    ├── HorizontalBox_Judge   （判定区图标）
    │    └── 3 个 Image（乐/兵/闪电）
    ├── TextBlock_HandCount   （手牌数 x N）
    └── Image_TurnIndicator   （当前回合高亮环，默认隐藏）

数据绑定:
  SetSeatData(FTKPlayerUIData)
    → 更新所有子控件
    → Image_TurnIndicator.Visibility = bIsCurrentTurn ? Visible : Hidden
    → Identity = bIdentityRevealed ? 显示身份 : "???"
```

#### Step 4: `WBP_SeatLayout`（围坐布局）

```
继承: UUserWidget

布局策略（圆桌布局）:
  2 人局: 上下对坐
  4 人局: 上下左右四方位
  5-8 人局: Canvas Panel + 三角函数计算位置

控件层级:
  Canvas Panel
    └── 动态放置 WBP_PlayerSeat 实例

事件:
  RefreshSeats(TArray<FTKPlayerUIData>)
    → 按 SeatIndex 排序
    → 计算每个座位 Canvas 位置（360°/N 均分圆周）
    → 更新每个 WBP_PlayerSeat 数据
```

#### Step 5: `WBP_EquipArea`（本地玩家装备区）

```
继承: UUserWidget

控件层级（水平排列 5 个槽位）:
  HorizontalBox
    ├── WBP_EquipSlot（武器 槽，标签"武器"）
    ├── WBP_EquipSlot（防具 槽，标签"防具"）
    ├── WBP_EquipSlot（+1马 槽）
    ├── WBP_EquipSlot（-1马 槽）
    └── WBP_EquipSlot（宝物 槽）

数据:
  从 FTKPlayerUIData.EquipmentCards 读取
```

#### Step 6: `WBP_ActionBar`（操作按钮栏）

```
继承: UUserWidget

控件层级:
  HorizontalBox
    ├── Button_UseCard      （使用卡牌）
    ├── Button_EndTurn      （结束回合）
    └── Button_Cancel       （取消选择）

可见性控制:
  → bIsPlayPhase && bIsMyTurn: 显示 使用/结束回合
  → SelectedCardIndex >= 0:     显示 使用/取消
  → 其他情况:                   仅显示 结束回合

事件:
  Button_EndTurn.OnClicked → OwningHUD → OnEndTurnClicked()
  Button_UseCard.OnClicked → OwningHUD → OnCardClicked(SelectedCardIndex)
```

#### Step 7: `WBP_ResponsePanel`（响应面板）

```
继承: UUserWidget

控件层级:
  Border（默认隐藏，Collapsed）
    ├── TextBlock_PromptText    （"你需要出一张【闪】"）
    ├── ProgressBar_Countdown   （倒计时进度条）
    ├── HorizontalBox_MyCards   （我的手牌，可出牌高亮）
    ├── Button_Submit           （出牌）
    └── Button_Pass             （放弃）

事件:
  OnResponseShown(Data)
    → SetVisibility(Visible)
    → 更新 PromptText = Data.PromptText
    → 启动倒计时 Timer

  Button_Submit.OnClicked
    → OwningHUD → OnResponseCardClicked(SelectedCardIndex)

  Button_Pass.OnClicked
    → OwningHUD → OnPassResponse()
```

#### Step 8: `WBP_GameHud`（主容器 — 组装所有子面板）

```
继承: UTKGameHudWidget

控件层级:
  Canvas Panel（全屏）
    ├── WBP_TopBar              （顶部居中）
    ├── WBP_SeatLayout          （中央圆桌区域）
    ├── WBP_EquipArea           （左下角，仅本地玩家）
    ├── WBP_HandArea            （底部居中，仅本地玩家手牌）
    ├── WBP_ActionBar           （右下角操作按钮）
    └── WBP_ResponsePanel       （全屏覆盖层，默认隐藏）

蓝图覆写事件:
  Event OnGameDataUpdated(GameData, PlayerData)
    → WBP_TopBar.update(GameData)
    → WBP_SeatLayout.RefreshSeats(PlayerData)

  Event OnMyHandCardsUpdated(Cards)
    → WBP_HandArea.OnHandCardsUpdated(Cards)

  Event OnActionStateUpdated(ActionData)
    → WBP_ActionBar.update(ActionData)

  Event OnResponseShown(Data)
    → WBP_ResponsePanel.SetVisibility(Visible)
    → WBP_ResponsePanel.update(Data)

  Event OnResponseHidden()
    → WBP_ResponsePanel.SetVisibility(Collapsed)
```

---

## 五、数据流完整链路（以"玩家点击手牌【杀】"为例）

```
1. 蓝图: WBP_CardItem Button.OnClicked
     ↓
2. UTKCardSlotWidget::OnCardClickedBP()
     ↓
3. ATKGameHUD::SelectCard(Index)
     → SelectedCardIndex = Index
     → OnCardSelectionChanged 广播 → WBP_CardItem 高亮边框
     ↓
4. 蓝图: WBP_ActionBar Button_UseCard.OnClicked
     ↓
5. ATKGameHUD::OnCardClicked(SelectedCardIndex)
     → 获取 Card = Hand[SelectedCardIndex]
     → PlayerController->ServerUseCard(Card)   // Server RPC
     ↓
6. 服务端: UTKCard_Basic::Use(User, Target)
     → 需要选择目标 → 发起响应窗口(ResponseComponent)
     ↓
7. 服务端 → 客户端: PlayerController->ClientPromptResponse(Tag, Text)
     ↓
8. ATKGameHUD::ShowResponsePanel(RequiredTag)
     → MainHud->SetResponseUIData(Data)
     → MainHud->ShowResponsePanel()
     → WBP_ResponsePanel 弹出
     ↓
9. 对手蓝图: 选择【闪】→ Submit → ServerSubmitResponse(Card)
     ↓
10. 服务端: ResponseComponent 结算 → 杀被抵消 → 进入弃牌堆
```

---

## 六、分阶段实施顺序

### 第一阶段（核心数据协议，预计 2-3 小时）

| 顺序 | 文件 | 操作 | 说明 |
|---|---|---|---|
| 1 | `Ui/TKGameUIData.h` | ✏️ 修改 | 新增 `FTKCardUIData`、`FTKActionUIData`；扩展 `FTKPlayerUIData` 加装备/判定区字段 |
| 2 | `Core/TKGameHUD.h` | ✏️ 修改 | 新增 `SelectedCardIndex`、`bHandCardsDirty`、`GetMyHandCardUIData()`、`GetActionUIData()`、`SelectCard()` |
| 3 | `Core/TKGameHUD.cpp` | ✏️ 修改 | 实现 `GetMyHandCardUIData()`、`GetActionUIData()`、改造 `Tick()` 为脏标记驱动 |
| 4 | `Ui/TKGameHudWidget.h` | ✏️ 修改 | 新增 `OnMyHandCardsUpdated`、`OnActionStateUpdated`、`OnCardSelectionChanged` BlueprintImplementableEvent |
| 5 | `Ui/TKGameHudWidget.cpp` | ✏️ 修改 | `RefreshDisplay` 中增加手牌刷新调用 |

### 第二阶段（蓝图 View 搭建，预计 4-6 小时）

| 顺序 | 蓝图 | 操作 | 关键内容 |
|---|---|---|---|
| 6 | `WBP_CardItem` | ✏️ 重写现有 | 绑定 `OnCardDataUpdated` 和 `OnCardClickedBP` 事件图，实现花色/点数/名称显示和高亮选中 |
| 7 | `WBP_HandArea` | 🆕 新建 | 动态生成 CardItem 列表，水平排列 |
| 8 | `WBP_PlayerSeat` | 🆕 新建 | 头像/身份/体力条/装备图标/判定区/手牌数/回合指示器 |
| 9 | `WBP_SeatLayout` | 🆕 新建 | 圆桌布局算法，按 SeatIndex 动态放置 PlayerSeat |
| 10 | `WBP_EquipArea` | 🆕 新建 | 5 槽位装备区 |
| 11 | `WBP_ActionBar` | 🆕 新建 | 使用/结束回合/取消三个按钮，阶段感知可见性 |
| 12 | `WBP_ResponsePanel` | 🆕 新建 | 提示文本/倒计时/可选牌/出牌/放弃 |
| 13 | `WBP_TopBar` | 🆕 新建 | 顶部信息栏 |
| 14 | `WBP_GameHud` | ✏️ 重写现有 | 组装所有子面板，绑定所有 `BlueprintImplementableEvent` |

### 第三阶段（联调与配置，预计 1-2 小时）

| 顺序 | 操作 |
|---|---|
| 15 | 在 `BP_GameHUD` 中设置 `HUDWidgetClass = WBP_GameHud` |
| 16 | `TKGameModeBase` 确认 `HUDClass = ATKGameHUD::StaticClass()`（已配好） |
| 17 | `lv_Test.umap` 编辑器中 PIE 测试，CheatManager 命令验证全流程 |

---

## 七、文件清单总结

| 文件 | 操作 | 说明 |
|---|---|---|
| `Source/TKGamea/Ui/TKGameUIData.h` | ✏️ 修改 | 追加 FTKCardUIData, FTKActionUIData, 扩展 FTKPlayerUIData |
| `Source/TKGamea/Core/TKGameHUD.h` | ✏️ 修改 | 追加选区管理、手牌推送、状态查询接口 |
| `Source/TKGamea/Core/TKGameHUD.cpp` | ✏️ 修改 | 改造 Tick、实现新方法 |
| `Source/TKGamea/Ui/TKGameHudWidget.h` | ✏️ 修改 | 追加新 BlueprintImplementableEvent |
| `Source/TKGamea/Ui/TKGameHudWidget.cpp` | ✏️ 修改 | 连线新事件 |
| `Content/Ui/HUD/WBP_GameHud.uasset` | ✏️ 重写 | 组装所有子面板 |
| `Content/Ui/W_CardItem.uasset` | ✏️ 重写 | 绑定数据事件 |
| `Content/Ui/WBP_HandArea` | 🆕 新建 | 手牌区容器 |
| `Content/Ui/WBP_PlayerSeat` | 🆕 新建 | 玩家座位 |
| `Content/Ui/WBP_SeatLayout` | 🆕 新建 | 圆桌布局 |
| `Content/Ui/WBP_EquipArea` | 🆕 新建 | 装备区 |
| `Content/Ui/WBP_ActionBar` | 🆕 新建 | 操作按钮栏 |
| `Content/Ui/WBP_ResponsePanel` | 🆕 新建 | 响应面板 |
| `Content/Ui/WBP_TopBar` | 🆕 新建 | 顶部信息栏 |

---

## 八、关键技术决策说明

| 决策 | 理由 |
|---|---|
| **FTKCardUIData 替代直接读 UTKCardBase\*** | P4 问题：UObject 属性不复制到客户端。FTKCardUIData 是 USTRUCT，数据由服务端 HUD 采集后随组件复制同步 |
| **Tick 改为脏标记驱动** | 当前每帧全量刷新，60fps 下大量冗余调用。改为仅在回合切换/阶段切换/手牌变化时推送对应数据 |
| **不直接在 Tick 中刷新全部数据** | 减少蓝图端每帧重绘开销，避免 Widget 闪烁 |
| **蓝图搭 View 不写 C++ Widget** | 按 MVC 设计文档约定，View 层完全用 Blueprint，方便调整布局和视觉 |
