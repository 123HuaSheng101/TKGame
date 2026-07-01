# TKGame UI 开发实施方案指南

## 1. 项目架构总览

TKGame（三国杀卡牌对战）采用 **MVC 架构**，C++ 侧实现了完整的 Controller 和 Model 层，View 层定义了接口但需要在蓝图中落地。

```
┌─────────────────────────────────────────────────────────────────────┐
│                           MVC 三层架构                               │
├───────────────┬───────────────────────┬──────────────────────────────┤
│  Model（数据） │   Controller（控制）   │       View（显示）            │
├───────────────┼───────────────────────┼──────────────────────────────┤
│ GameStateBase │  ATKGameHUD           │  UTKGameHudWidget（主 HUD）  │
│ PlayerState   │  (Tick 采集数据)       │  UTKCardSlotWidget（卡牌Slot）│
│ CardZoneComp  │  (刷新 View)           │  UTKDebugHudWidget（调试）    │
│ DeckComponent │  (接收交互回调)         │                              │
│ TurnComponent │  (Server RPC)          │                              │
│ ResponseComp  │                       │                              │
└───────────────┴───────────────────────┴──────────────────────────────┘
```

### 数据流向

```
GameState / PlayerState / CardZone / TurnComp
         │
         ▼  Tick 每帧采集
    ATKGameHUD (Controller)
         │
         ├──► RefreshDisplay(FTKGameUIData, TArray<FTKPlayerUIData>)
         │          └──► OnGameDataUpdated (蓝图事件)
         │
         ├──► RefreshHandCards()
         │          └──► OnHandCardsUpdated (蓝图事件)
         │
         └──► ShowResponsePanel() / HideResponsePanel()
                    └──► OnResponseShown / OnResponseHidden (蓝图事件)

用户点击 ──► OnCardClicked / OnResponseCardClicked / OnEndTurnClicked
                     │
                     ▼  Server RPC
              ServerUseCard / ServerSubmitResponse / ServerAdvancePhase
```

---

## 2. 已完成项（C++ 侧）

| 链路 | 涉及文件 | 说明 |
|------|----------|------|
| **数据采集** | `TKGameHUD.cpp` | `Tick` → `GetGameUIData()` + `GetAllPlayerUIData()` → `RefreshDisplay()` |
| **响应面板推送** | `TKPlayerControllerBase.cpp` | `ClientPromptResponse` RPC → `ATKGameHUD::ShowResponsePanel()` → `OnResponseShown` |
| **卡牌使用** | `TKGameHUD.cpp` | `OnCardClicked()` → `ServerUseCard` RPC |
| **响应提交** | `TKGameHUD.cpp` | `OnResponseCardClicked()` → `ServerSubmitResponse` RPC |
| **结束回合** | `TKGameHUD.cpp` | `OnEndTurnClicked()` → `ServerAdvancePhase` RPC |
| **跳过响应** | `TKGameHUD.cpp` | `OnPassResponse()` → `HideResponsePanel()` |
| **数据结构** | `TKGameUIData.h` | `FTKGameUIData` / `FTKPlayerUIData` / `FTKResponseUIData` |

---

## 3. 蓝图实施路线图

### 3.1 P0（核心交互）— 创建 WBP_CardSlot 蓝图

> 目标：单张手牌的 UI 展示和点击交互

**父类**：`UTKCardSlotWidget`

**UMG 布局建议**：

```
Border（根，可选高亮边框）
└─ Overlay
   ├─ Image_CardFace（卡面）
   ├─ Text_CardName（卡牌名称）
   ├─ Text_SuitAndValue（花色+点数）
   └─ Button_Click（全尺寸透明按钮，捕获点击）
```

**蓝图事件实现**：

| 事件 | 逻辑 |
|------|------|
| `OnCardDataUpdated(Card)` | 从 `Card` 读取 `CardName`、`CardSuit`、`CardValue` 更新 UI 控件 |
| `OnCardClickedBP` | 可在此加点击音效/动画，然后通过 `OnClicked` 委托通知 HUD |
| `Button_Click.OnClicked` | 调用 `OnCardClicked.ExecuteIfBound(CardIndex)` 或直接在 HUD 处理 |

**关键点**：`UTKCardSlotWidget` 已持有 `CardData`（`TObjectPtr<UTKCardBase>`）和 `CardIndex`，蓝图可直接读取做显示。

---

### 3.2 P0（核心交互）— 创建 WBP_GameHud 蓝图

> 目标：主 HUD 容器，管理手牌区域、响应面板、玩家信息

**父类**：`UTKGameHudWidget`

**UMG 布局建议**：

```
Canvas Panel（根）
│
├─ [左上角] WBP_DebugHud（调试覆盖层，可选显示/隐藏）
│
├─ [居中] Border_ResponsePanel（响应面板，Visibility=Collapsed）
│   ├─ VerticalBox
│   │   ├─ Text_PromptText（例如："需要出 [闪] "）
│   │   ├─ Text_Timer（倒计时 10s → 0s）
│   │   ├─ WrapBox_ResponseCards（可选出的牌列表，动态生成 CardSlot）
│   │   └─ Button_PassResponse（跳过按钮）
│   │
│   └─ （关闭后 Visibility=Collapsed）
│
├─ [四周] 玩家座位环（见 3.3 节）
│
├─ [底部] Border_HandArea
│   └─ HorizontalBox_HandContainer（手牌容器）
│
└─ [右下角] Button_EndTurn（结束回合按钮）
```

**蓝图事件实现**：

#### `OnGameDataUpdated(GameData, PlayerData)`

```
// 1. 更新顶部状态栏
Text_TurnInfo    ← "第 {GameData.TurnNumber} 回合"
Text_PhaseName   ← "{GameData.PhaseName}"
Text_CurrentPlayer ← "{GameData.CurrentPlayerName}"
Text_DeckInfo    ← "牌堆: {GameData.DeckCount} / 弃牌: {GameData.DiscardCount}"

// 2. 检查游戏是否结束
If GameData.WinnerFaction != 0:
   Show GameOver Panel

// 3. 遍历 PlayerData 更新所有玩家座位
ForEach Player in PlayerData:
   FindSeatWidget(Player.SeatIndex) → 更新名称/HP/手牌数/高亮
```

#### `OnHandCardsUpdated(Cards)`

```
// 清空旧手牌
HandCardContainer.ClearChildren()

// 遍历生成新手牌
For i = 0 to Cards.Length - 1:
   SlotWidget = CreateWidget(WBP_CardSlot)
   SlotWidget.SetCardData(Cards[i], i)
   HandCardContainer.AddChild(SlotWidget)
```

#### `OnResponseShown(Data)`

```
ResponsePanel.SetVisibility(Visible)
Text_PromptText ← Data.PromptText
Text_Timer ← FormatText("{0}s", Data.TimeRemaining)

// 如果是响应面板，还需要根据当前手牌过滤可响应牌（高亮可用牌）
// 例如：高亮所有带 "GameplayTags.Card.Response.Dodge" Tag 的牌
```

#### `OnResponseHidden()`

```
ResponsePanel.SetVisibility(Collapsed)
WrapBox_ResponseCards.ClearChildren()
```

#### 按钮绑定

| 按钮 | 绑定事件 |
|------|----------|
| `Button_EndTurn.OnClicked` | → `GetOwningHUD().OnEndTurnClicked()` |
| `Button_PassResponse.OnClicked` | → `GetOwningHUD().OnPassResponse()` |

---

### 3.3 P1（游戏信息）— 玩家座位 Widget

> 目标：展示每个玩家的名称/身份/HP/手牌数/回合高亮

**方案一（推荐）**：创建 `WBP_PlayerSeat` 蓝图（父类 `UUserWidget`）

**UMG 布局**：

```
SizeBox（固定尺寸 200x80）
└─ Border（背景色，当前回合玩家高亮）
   └─ VerticalBox
      ├─ Text_PlayerName（玩家名）
      ├─ HorizontalBox
      │   ├─ Text_Identity（身份，如"主公"/"反贼"）
      │   └─ Text_HP（HP 显示 "3/4"）
      ├─ Text_HandCount（手牌数 "× 4"）
      └─ Image_TurnIndicator（当前回合指示器）
```

**公开函数**（在蓝图接口中定义）：

```
SetSeatData(PlayerName, IdentityName, bAlive, CurrentHP, MaxHP, HandCount, bIsCurrentTurn):
   Text_PlayerName ← PlayerName
   Text_Identity ← IdentityName
   Text_HP ← "{CurrentHP}/{MaxHP}"
   Text_HandCount ← "× {HandCount}"
   
   // 死亡处理
   If NOT bAlive:
      SetColorAndOpacity = Grey
      Text_HP = "阵亡"
   
   // 当前回合高亮
   If bIsCurrentTurn:
      Border.SetBrushColor = Gold/Yellow
   Else:
      Border.SetBrushColor = Default
```

**方案二**：使用 3D Widget Component 挂载到角色头顶（适合有 3D 场景时）

---

### 3.4 P1（响应系统）— 响应面板完善

> 目标：完整实现杀→闪、南蛮入侵→杀 等响应流程

**数据流回顾**：

```
服务端发起 RequestResponse
  → ResponseComponent::BroadcastRequestToClients()
    → 各客户端 PlayerController::ClientPromptResponse(RequiredTag, PromptText)
      → ATKGameHUD::ShowResponsePanel(RequiredTag)
        → MainHud->SetResponseUIData(...)
          → 蓝图 OnResponseShown 事件
```

**蓝图逻辑细化**：

```
OnResponseShown(Data):
   // 1. 显示面板
   ResponsePanel.Visibility = Visible
   PromptText ← Data.PromptText     // 例: "需要打出 [闪]"
   RequiredTag ← Data.RequiredTag   // 例: "Card.Response.Dodge"
   
   // 2. 倒计时更新（Tick 中）
   Text_Timer ← "{FMath::CeilToInt(Data.TimeRemaining)}s"
   
   // 3. 过滤手牌：高亮可用牌，置灰不可用牌
   ForEach CardWidget in HandCardContainer.GetAllChildren():
      CardObject = CardWidget.GetCard()
      If CardObject.GameplayTags.Contains(RequiredTag):
         CardWidget.SetHighlight(true)
         CardWidget.OnClicked → 高亮牌点击 → GetOwningHUD().OnResponseCardClicked(Index)
      Else:
         CardWidget.SetOpacity(0.3)  // 置灰
   
   // 4. 跳过按钮
   Button_PassResponse.OnClicked → GetOwningHUD().OnPassResponse()
```

---

### 3.5 P2（视觉完善）— 座位环形布局

> 目标：按三国杀风格将玩家座位排列成环形

**实现方式**：在 `WBP_GameHud` 蓝图中预置固定位置的 `WBP_PlayerSeat` Widget

```
座位布局（以 8 人局为例）：

                   [主视角] ← 底部居中，无需显示自己
         [座1]                    [座7]
      [座2]                          [座6]
         [座3]                    [座5]
                   [座4]

方案：
- 对于 N 人局，用 Canvas Panel + Render Translation 计算各座位角度
- 公式：angle = 2π * SeatIndex / TotalPlayers
- PosX = CenterX + R * sin(angle)
- PosY = CenterY - R * cos(angle)
```

---

## 4. 配置步骤

### 4.1 创建蓝图类

在 Unreal Editor 中：

1. **Content Browser** → 右键 → `Blueprint Class`
2. 搜索并选择 C++ 父类：
   - `TKGameHudWidget` → 命名为 `WBP_GameHud`
   - `TKCardSlotWidget` → 命名为 `WBP_CardSlot`
3. 打开蓝图 → Event Graph → 覆写 `BlueprintImplementableEvent`

### 4.2 配置 HUD Widget Class

在 `BP_TKGameHUD`（如有）或直接在 `ATKGameHUD` C++ 类默认值中：

```
HUDWidgetClass = WBP_GameHud
```

或在 GameMode 蓝图中配置 `HUD Class`。

### 4.3 调试运行

1. PIE 模式启动
2. 按 `~` 打开控制台
3. 输入 Cheat：`tk.StartGame`（启动身份局）
4. 输入 Cheat：`tk.AdvancePhase`（推进阶段测试）
5. 观察手牌面板是否显示卡牌

---

## 5. 完整文件清单

### 已有 C++ 文件（无需修改）

| 文件 | 角色 |
|------|------|
| `Ui/TKGameUIData.h` | UI 数据结构定义 |
| `Ui/TKUserWidgetBase.h/.cpp` | Widget 基类 |
| `Ui/TKGameHudWidget.h/.cpp` | 主 HUD View 接口 |
| `Ui/TKCardSlotWidget.h/.cpp` | 卡牌 Slot View 接口 |
| `Ui/TKDebugHudWidget.h/.cpp` | 调试 HUD |
| `Core/TKGameHUD.h/.cpp` | HUD Controller |

### 需要新建的蓝图

| 蓝图名 | 父类 | 用途 |
|--------|------|------|
| `WBP_GameHud` | `UTKGameHudWidget` | 主 HUD 容器 |
| `WBP_CardSlot` | `UTKCardSlotWidget` | 单张手牌显示 |
| `WBP_PlayerSeat` | `UUserWidget` | 玩家信息展示 |
| `WBP_DebugHud` | `UTKDebugHudWidget` | 调试覆盖层（可选复用） |

---

## 6. 注意事项

### 6.1 网络复制问题

当前 `OnHandCardsUpdated` 传递的是 `TArray<UTKCardBase*>`（服务器端 UObject 引用）。在多人网络环境中，客户端**无法直接访问**服务器上的 UObject。

- **单机/PIE 调试**：当前代码可以直接使用
- **多人网络**：需要将 `ReplicatedHandCards`（`TArray<FTKCardInstance>`）作为客户端手牌数据源，因为 `FTKCardInstance` 是 `USTRUCT`，天然支持网络复制

### 6.2 手牌容器每帧重建 vs 增量更新

当前 `OnHandCardsUpdated` 建议走 **全量重建**（`ClearChildren` + 重新 `CreateWidget`），因为简单且不容易出错。如果后续手牌数量大（>20张）考虑改为增量更新（Diff + Add/Remove）。

### 6.3 蓝图性能

- `OnGameDataUpdated` 每帧调用，避免在其中做复杂计算
- 玩家座位信息更新建议用变量变化检测（`!=` 比较避免重复 set）
- 手牌刷新不必每帧（可以在 `NativeTick` 里检查 `HandCards.Num()` 变化后再刷）

---

## 7. 优先级总结

| 优先级 | 任务 | 产出 | 依赖 |
|--------|------|------|------|
| 🔴 P0 | 创建 `WBP_CardSlot` | 卡牌可见 | 无 |
| 🔴 P0 | 创建 `WBP_GameHud` + 手牌 | 手牌可看可点 | WBP_CardSlot |
| 🟡 P1 | 实现 `OnGameDataUpdated` | 玩家座位/回合信息可见 | WBP_PlayerSeat |
| 🟡 P1 | 响应面板完整实现 | 杀→闪 等交互闭环 | WBP_CardSlot |
| 🟢 P2 | 座位环形布局 | 视觉完善 | WBP_PlayerSeat |
| 🟢 P2 | 调试 HUD 覆盖 | 开发效率 | 可并行 |
