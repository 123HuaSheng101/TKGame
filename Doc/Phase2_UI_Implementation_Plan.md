# Phase 2：游戏 UI 实现方案

> 日期：2026-06-29 | 基于当前代码版本（Phase 1 完成）
> 目标：使游戏可通过鼠标操作进行完整对战

---

## 一、现有 UI 架构回顾

### 1.1 已有基础设施 ✅

```
┌─────────────────────────────────────────────────────────┐
│  ATKGameHUD (Controller)                                │
│  • Tick() 每帧采集数据 → Push 到 View                    │
│  • ShowResponsePanel(RequiredTag) / HideResponsePanel() │
│  • OnCardClicked / OnResponseCardClicked / OnPassResponse│
│  • OnEndTurnClicked → ServerAdvancePhase                │
│  • GetMyHandCards() / GetGameUIData() / GetAllPlayerUIData()│
├─────────────────────────────────────────────────────────┤
│  UTKGameHudWidget (View)                                │
│  • RefreshDisplay(GameData, PlayerData[]) — BP事件      │
│  • RefreshHandCards() — BP事件                          │
│  • Show/HideResponsePanel() — BP事件                    │
│  • 所有视觉委托给 BlueprintImplementableEvent           │
├─────────────────────────────────────────────────────────┤
│  UTKCardSlotWidget (卡牌View)                           │
│  • SetCardData(Card, Index) → OnCardDataUpdated BP事件  │
│  • OnClicked 委托 + OnCardClickedBP BP事件              │
├─────────────────────────────────────────────────────────┤
│  ATKPlayerControllerBase (RPC层)                        │
│  • ServerUseCard(Card) / ServerSubmitResponse(Card)     │
│  • ServerAdvancePhase() / ServerStartGame()             │
│  • ClientPromptResponse(Tag, PromptText) ← 从未被调用！ │
└─────────────────────────────────────────────────────────┘
```

### 1.2 UI 数据协议 ✅

| 结构体 | 用途 | 状态 |
|--------|------|------|
| `FTKGameUIData` | 回合/阶段/牌堆数/胜负 | ✅ 已有 |
| `FTKPlayerUIData` | 座位/名称/身份/HP/手牌数/是否当前回合 | ✅ 已有 |
| `FTKResponseUIData` | 需要的Tag/提示文本/倒计时 | ✅ 已有 |
| `FTKCardInstance` | 客户端可读的卡牌数据（花色/点数/类型/名称） | ✅ Phase1 已实现 |

---

## 二、关键缺口：C++ 侧必须修复的 4 个问题

### 🚨 缺口 A：事件广播不到客户端（最高优先阻塞）

**现状**：
- `ResponseComponent::BroadcastRequestToClients()` 通过 EventComponent 发布本地事件
- EventComponent 的 `PostEvent()` 只做 `HandleLocalEvent`，**不会**发到其他客户端
- `ClientPromptResponse` RPC 已定义但从未被调用
- 结果：客户端永远不知道需要出牌响应

**修复方案**（`TKResponseComponent.cpp`）：

在 `BroadcastRequestToClients()` 中，根据响应类型直接调用目标客户端的 RPC：

```cpp
// Sequential 模式：通知当前需要响应的那个玩家
if (ActiveRequest.Type == ETKResponseType::Sequential || 
    ActiveRequest.Type == ETKResponseType::SingleTarget)
{
    if (ActiveRequest.PrimaryTarget)
    {
        APlayerController* PC = ActiveRequest.PrimaryTarget->GetPlayerController();
        if (ATKPlayerControllerBase* TKPC = Cast<ATKPlayerControllerBase>(PC))
        {
            FString TagStr = ActiveRequest.RequiredTag.GetTagName().ToString();
            FText Prompt = FText::FromString(FString::Printf(
                TEXT("请出一张 %s"), *TagStr));
            TKPC->ClientPromptResponse(ActiveRequest.RequiredTag, Prompt);
        }
    }
}

// Chain 模式（无懈可击）：通知所有存活玩家
if (ActiveRequest.Type == ETKResponseType::Chain)
{
    TArray<APlayerState*> Alive = GetAlivePlayers();
    for (APlayerState* PS : Alive)
    {
        APlayerController* PC = PS->GetPlayerController();
        if (ATKPlayerControllerBase* TKPC = Cast<ATKPlayerControllerBase>(PC))
        {
            TKPC->ClientPromptResponse(ActiveRequest.RequiredTag,
                FText::FromString(TEXT("是否使用无懈可击？")));
        }
    }
}
```

### 🚨 缺口 B：ServerUseCard 不支持目标选择

**现状**：`ServerUseCard_Implementation` 硬编码 `Card->Use(this, UserPS)` — 目标永远是**自己**。

**修复**：

```cpp
// TKPlayerControllerBase.h — 新增
UFUNCTION(Server, Reliable, WithValidation)
void ServerUseCardOnTarget(UTKCardBase* Card, ATKPlayerStateBase* Target);

// TKPlayerControllerBase.cpp — 实现
void ATKPlayerControllerBase::ServerUseCardOnTarget_Implementation(
    UTKCardBase* Card, ATKPlayerStateBase* Target)
{
    if (!Card || !Target) return;
    Card->Use(this, Target);
}
```

### 🔶 缺口 C：手牌数据过滤（响应时高亮可用牌）

客户端需要知道"哪些手牌可以出"来高亮。在 `ATKGameHUD` 中添加过滤方法：

```cpp
// TKGameHUD.h — 新增
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HUD")
TArray<int32> GetValidResponseCardIndices(const FGameplayTag& RequiredTag) const;

// TKGameHUD.cpp — 实现
TArray<int32> ATKGameHUD::GetValidResponseCardIndices(const FGameplayTag& RequiredTag) const
{
    TArray<int32> Result;
    TArray<UTKCardBase*> Hand = GetMyHandCards();
    for (int32 i = 0; i < Hand.Num(); i++)
    {
        if (Hand[i] && Hand[i]->EffectTags.Num() > 0)
        {
            // 匹配 RequiredTag，或濒死时酒也可
            if (Hand[i]->EffectTags[0] == RequiredTag) Result.Add(i);
            // 酒代桃的特殊匹配
            static FGameplayTag TagPeach = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Peach"), false);
            static FGameplayTag TagWine  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Wine"), false);
            if (RequiredTag == TagPeach && Hand[i]->EffectTags[0] == TagWine) Result.Add(i);
        }
    }
    return Result;
}
```

### 🔶 缺口 D：BufferedResponseData 供 Widget 查询

当前 `ShowResponsePanel` 传入的 `FTKResponseUIData` 被传给 Widget 但不保存，Widget 后续无法查询"当前是否需要响应"、"需要什么 Tag"。

**修复**：在 ATKGameHUD 中添加：

```cpp
// TKGameHUD.h — 新增成员
UPROPERTY(BlueprintReadOnly, Category = "HUD")
FTKResponseUIData CurrentResponseData;

// 查询当前是否在等待响应
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HUD")
bool IsWaitingForResponse() const { return CurrentResponseData.bIsWaiting; }
```

`ShowResponsePanel` 中写入 `CurrentResponseData`，`HideResponsePanel` 中重置。

---

## 三、蓝图侧：UE 编辑器中创建 Widget

这是本阶段主要工作量。**所有 C++ 接口已就绪，蓝图侧只需组合布局**。

### 3.1 创建 WBP_CardSlot（卡牌槽位 Widget）

**父类**：`UTKCardSlotWidget`

| 控件 | 类型 | 说明 |
|------|------|------|
| `CardBorder` | Border | 卡牌边框（选中时高亮） |
| `CardNameText` | TextBlock | 绑定 `CardData.CardName` |
| `SuitIcon` | Image | 绑定花色图标（♠♥♣♦） |
| `RankText` | TextBlock | 绑定 `CardData.Rank` |
| `TypeIcon` | Image | 基本/锦囊/装备图标 |

**蓝图逻辑**：
- `OnCardDataUpdated`：根据 CardData 刷新所有子控件
- `OnCardClickedBP`：触发 OnCardClicked 委托 + 边框高亮切换

### 3.2 创建 WBP_GameHud（主 HUD Widget）

**父类**：`UTKGameHudWidget`  
**画布大小**：全屏（1920x1080 适配）

#### 区域布局

```
┌──────────────────────────────────────────────────┐
│  [回合/阶段横幅]  Turn 3 · Play Phase             │
│  [当前玩家: PlayerName]                           │
├──────────────────────┬───────────────────────────┤
│                      │                           │
│   【其他玩家区】     │                           │
│   ┌──────────────┐  │                           │
│   │ [头像] P2    │  │                           │
│   │ HP: ████░░   │  │      【中央游戏区】        │
│   │ 手牌: 4      │  │                           │
│   │ 装备: 🗡🛡   │  │   牌堆: 48   弃牌堆: 12   │
│   │ 🟢当前回合   │  │                           │
│   └──────────────┘  │                           │
│   ┌──────────────┐  │                           │
│   │ [头像] P3    │  │                           │
│   │ ...          │  │                           │
│   └──────────────┘  │                           │
│                      │                           │
├──────────────────────┴───────────────────────────┤
│                                                  │
│   【我的手牌区】  HorizontalBox                   │
│   [杀♠7] [闪♥2] [桃♥3] [闪♦2] [杀♣7] [无懈♣Q]  │
│                                                  │
│   【操作按钮】  [结束出牌]                        │
└──────────────────────────────────────────────────┘
```

#### 控件清单

| 区域 | 控件 | 类型 | 说明 |
|------|------|------|------|
| **顶部横幅** |
| | TurnText | TextBlock | "Turn {TurnNumber}" |
| | PhaseText | TextBlock | "Play Phase" |
| | DeckInfo | TextBlock | "牌堆: {DeckCount}  弃牌: {DiscardCount}" |
| **玩家列表** |
| | PlayerPanel | VerticalBox | 包裹所有玩家条目 |
| | PlayerEntry (×n) | WBP_PlayerEntry | 头像+HP+手牌数 |
| **我的手牌** |
| | HandCardBox | HorizontalBox | 动态填充 WBP_CardSlot |
| **操作** |
| | EndTurnBtn | Button | "结束出牌" |
| **响应弹窗** |
| | ResponseOverlay | Overlay | 默认隐藏 |
| | ResponseText | TextBlock | "请出一张杀！" |
| | ResponseTimer | TextBlock | 倒计时 "8s" |
| | PassBtn | Button | "不出" |

#### 蓝图事件实现

##### OnGameDataUpdated(GameData, PlayerData[])

```
1. TurnText ← "Turn " + GameData.TurnNumber
2. PhaseText ← GameData.PhaseName
3. DeckInfo ← "牌堆: " + GameData.DeckCount + "  弃牌: " + GameData.DiscardCount
4. 遍历 PlayerData 数组：
   - 创建/更新 PlayerPanel 中的 WBP_PlayerEntry
   - 刷新 HP 条、手牌数、身份（死后显示）
   - 高亮 CurrentPlayer 的边框
5. 检查 GameData.WinnerFaction != 0 → 显示胜负数
```

##### OnHandCardsUpdated(Cards[])

```
1. 清空 HandCardBox
2. 遍历 Cards：
   - 创建 WBP_CardSlot 实例
   - SetCardData(Card, Index)
   - 绑定 OnClicked 委托到 ATKGameHUD::OnCardClicked
   - 添加到 HandCardBox
```

##### OnResponseShown(Data)

```
1. ResponseOverlay.SetVisibility(Visible)
2. ResponseText ← Data.PromptText
3. 高亮手牌中匹配 RequiredTag 的卡牌（调用 GetValidResponseCardIndices）
4. 启动倒计时定时器
```

##### OnResponseHidden()

```
1. ResponseOverlay.SetVisibility(Collapsed)
2. 清除所有手牌高亮
3. 停止倒计时定时器
```

### 3.3 创建 WBP_PlayerEntry（玩家状态条目）

这是个辅助 Widget，被 `WBP_GameHud` 的 PlayerPanel 引用。

| 控件 | 类型 | 说明 |
|------|------|------|
| PlayerBorder | Border | 边框（当前回合高亮） |
| AvatarImage | Image | 头像占位 |
| NameText | TextBlock | 玩家名 |
| HPBar | ProgressBar | 血量百分比 |
| HPText | TextBlock | "3/4" |
| IdentityText | TextBlock | 身份（仅自己可见/死后可见） |
| HandCountText | TextBlock | 手牌数 |
| EquipIcons | HorizontalBox | 装备图标 |
| DeadOverlay | Overlay | 死亡遮罩 |

---

## 四、实施步骤

### Step 0：UE 编辑器准备（5分钟）

1. 打开 `TKGamea.uproject`
2. 确认 `Content/Blueprints/` 目录存在
3. 找到 `B_GameMode_Test` 蓝图，确认其 `HUD Class` 设为 `ATKGameHUD`

### Step 1：修复 C++ 缺口（1h）🔴

| 文件 | 修改 |
|------|------|
| `TKResponseComponent.cpp` | `BroadcastRequestToClients` 中调用 `ClientPromptResponse` |
| `TKPlayerControllerBase.h/.cpp` | 新增 `ServerUseCardOnTarget` RPC |
| `TKGameHUD.h/.cpp` | 新增 `GetValidResponseCardIndices`、`CurrentResponseData`、`IsWaitingForResponse` |

### Step 2：创建蓝图 Widget（3-4h）🟡

| 顺序 | 蓝图 | 父类 | 预估 |
|------|------|------|------|
| 2a | `WBP_CardSlot` | `UTKCardSlotWidget` | 0.5h |
| 2b | `WBP_PlayerEntry` | `UserWidget` | 0.5h |
| 2c | `WBP_GameHud` | `UTKGameHudWidget` | 2h |
| 2d | 在 `B_GameMode_Test` 中设置 `HUDWidgetClass = WBP_GameHud` | — | 0.1h |

### Step 3：集成测试（1h）🟡

```
1. PIE 多窗口启动 4 人局
2. StartDebugGame
3. 验证：
   - 手牌正常显示（花色/点数/名称正确）
   - 点击手牌能使用
   - 杀选中后需要选目标（距离限制）
   - AOE 时响应弹窗弹出
   - 出杀/闪后弹窗关闭
   - 乐不思蜀判定后有"跳过出牌"提示
   - EndTurn 按钮推进阶段
```

### Step 4：视觉打磨（1-2h 可选）🟢

- 花色图标替换（♠♥♣♦ → 图片素材）
- 装备图标
- 动画过渡

---

## 五、总工时估算

| 类别 | 任务 | 预估 |
|------|------|------|
| C++ | 缺口 A-D 修复 | 1h |
| 蓝图 | WBP_CardSlot | 0.5h |
| 蓝图 | WBP_PlayerEntry | 0.5h |
| 蓝图 | WBP_GameHud | 2h |
| 测试 | 集成测试 | 1h |
| 打磨 | 视觉优化 | 1.5h |
| **合计** | | **6 ~ 6.5h** |

---

## 六、涉及文件清单

### C++ 修改

| 文件 | 操作 |
|------|------|
| `Core/TKResponseComponent.cpp` | 修改 `BroadcastRequestToClients` |
| `Core/TKPlayerControllerBase.h` | 新增 `ServerUseCardOnTarget` RPC |
| `Core/TKPlayerControllerBase.cpp` | 实现 `ServerUseCardOnTarget` |
| `Core/TKGameHUD.h` | 新增 `GetValidResponseCardIndices`、`CurrentResponseData`、`IsWaitingForResponse` |
| `Core/TKGameHUD.cpp` | 实现以上方法 + 修改 `ShowResponsePanel` |

### 蓝图新建

| 资源路径 | 名称 | 父类 |
|----------|------|------|
| `Content/Blueprints/` | `WBP_CardSlot` | `UTKCardSlotWidget` |
| `Content/Blueprints/` | `WBP_PlayerEntry` | `UserWidget` |
| `Content/Ui/` | `WBP_GameHud` | `UTKGameHudWidget` |

### 蓝图修改

| 文件 | 修改 |
|------|------|
| `B_GameMode_Test` | `HUDWidgetClass` 设为 `WBP_GameHud` |

---

## 七、M2 交付标准

Phase 2 完成后，应能通过 UI 完成以下操作：

1. ✅ 启动游戏后，看到全屏 HUD（回合/阶段信息、玩家状态、手牌）
2. ✅ 点击手牌选中一张牌（高亮边框）
3. ✅ 选牌后点击目标玩家头像出牌（杀→目标需在攻击范围内）
4. ✅ AOE/濒死时自动弹出响应面板，可用手牌高亮
5. ✅ 点击高亮手牌完成响应，或点击"不出"跳过
6. ✅ 点击"结束出牌"推进到弃牌阶段
7. ✅ 死亡玩家显示灰显遮罩 + 身份揭示
8. ✅ 游戏结束时显示胜负结果
