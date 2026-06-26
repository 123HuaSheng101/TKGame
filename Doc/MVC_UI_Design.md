# TKGame MVC UI 框架设计文档

## 架构概览

```
┌─────────────────────────────────────────────────┐
│              ATKGameHUD (Controller)             │
│  挂在 PlayerController 上（UE 原生 AHUD）         │
│  · NativeTick: Model → UIData → View             │
│  · 用户交互 → Server RPC                         │
│  · 响应面板生命周期管理                           │
└──────┬──────────────────────────┬───────────────┘
       │ Push Data                │ Call RPC
       ▼                          ▼
┌──────────────┐         ┌────────────────────┐
│   View 层     │         │     Model 层         │
│ (Blueprint)   │         │  (C++, 已完成)       │
│              │         │                    │
│ WBP_GameHud  │         │ ATKGameStateBase   │
│ WBP_CardSlot │         │ ATKPlayerStateBase │
│              │         │ UTKCardZoneComponent│
│              │         │ UTKDeckComponent   │
│              │         │ UTKTurnComponentBase│
└──────────────┘         └────────────────────┘
```

## 文件清单

### 新增文件

| 文件 | 层 | 说明 |
|------|-----|------|
| `Ui/TKGameUIData.h` | 协议 | `FTKGameUIData` / `FTKPlayerUIData` / `FTKResponseUIData` 结构体 |
| `Core/TKGameHUD.h/.cpp` | Controller | `ATKGameHUD : AHUD`，数据采集 + View 驱动 |
| `Ui/TKGameHudWidget.h/.cpp` | View | 主 HUD 接口，`BlueprintImplementableEvent` 供蓝图覆写 |
| `Ui/TKCardSlotWidget.h/.cpp` | View | 手牌卡牌接口 |

### 修改文件

| 文件 | 改动 |
|------|------|
| `Core/TKGameModeBase.cpp` | 注册 `HUDClass = ATKGameHUD::StaticClass()` |
| `Core/TKPlayerControllerBase.cpp` | `ClientPromptResponse` 串联 `ATKGameHUD::ShowResponsePanel()` |

## 数据流

### 每帧刷新

```
ATKGameHUD::NativeTick()
  ├── GetGameUIData()     → FTKGameUIData
  ├── GetAllPlayerUIData() → TArray<FTKPlayerUIData>
  └── MainHud->RefreshDisplay(GameData, PlayerData)
        └── OnGameDataUpdated (BlueprintImplementableEvent)
```

### 用户点击手牌

```
Blueprint: 卡牌按钮 OnClicked
  → C++: UTKCardSlotWidget::OnCardButtonClicked()
    → FOnCardClicked 委托
      → ATKGameHUD::OnCardClicked(CardIndex)
        → GetMyHandCards()
        → PC->ServerUseCard(Card)
```

### 响应提示

```
Server: ResponseComponent 广播
  → ATKPlayerControllerBase::ClientPromptResponse(Tag, Text)
    → ATKGameHUD::ShowResponsePanel(Tag)
      → MainHud->SetResponseUIData(Data)
      → MainHud->ShowResponsePanel()
        → OnResponseShown (BlueprintImplementableEvent)
```

### 用户提交响应

```
Blueprint: 响应牌按钮 OnClicked
  → ATKGameHUD::OnResponseCardClicked(CardIndex)
    → PC->ServerSubmitResponse(Card)
    → HideResponsePanel()
      → OnResponseHidden (BlueprintImplementableEvent)
```

### 蓝图事件绑定

```
ATKGameHUD::OnCardClicked(CardIndex)     ← Blueprint 按钮点击调用
ATKGameHUD::OnResponseCardClicked(CardIndex) ← Blueprint 响应牌点击调用
ATKGameHUD::OnPassResponse()            ← Blueprint Pass 按钮调用
ATKGameHUD::OnEndTurnClicked()          ← Blueprint EndTurn 按钮调用

Blueprint 覆写事件：
  OnGameDataUpdated(GameData, PlayerData)  ← 每帧数据推送
  OnHandCardsUpdated(Cards)                ← 手牌变化时
  OnResponseShown(Data)                    ← 响应面板弹出
  OnResponseHidden()                       ← 响应面板关闭
```

## Blueprint 搭建步骤

1. 创建 `WBP_GameHud` 继承 `UTKGameHudWidget`
   - 覆写 `OnGameDataUpdated` — 更新顶部信息栏、玩家状态表
   - 覆写 `OnHandCardsUpdated` — 生成 `WBP_CardSlot` 子控件
   - 覆写 `OnResponseShown` / `OnResponseHidden` — 切换响应面板

2. 创建 `WBP_CardSlot` 继承 `UTKCardSlotWidget`
   - 覆写 `OnCardDataUpdated` — 设置卡牌名称/花色/点数文本
   - 覆写 `OnCardClickedBP` — 调用 `GetOwningHUD()->OnCardClicked(GetCardIndex())`

3. 配置 `ATKGameHUD` 的 `HUDWidgetClass = WBP_GameHud`

4. 配置 `ATKGameModeBase`（已完成，`HUDClass = ATKGameHUD::StaticClass()`）
