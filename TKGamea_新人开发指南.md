# TKGamea 新人开发指南

> **项目名称**: TKGamea — 基于 Unreal Engine 5.6 的三国杀卡牌游戏  
> **语言**: C++ / Blueprint  
> **版本**: UE 5.6  
> **最后更新**: 2026-06-29

---

## 目录

1. [项目概述](#1-项目概述)
2. [环境搭建](#2-环境搭建)
3. [项目结构总览](#3-项目结构总览)
4. [核心架构](#4-核心架构)
5. [关键系统详解](#5-关键系统详解)
   - [5.1 游戏启动流程](#51-游戏启动流程)
   - [5.2 回合状态机](#52-回合状态机)
   - [5.3 响应系统](#53-响应系统)
   - [5.4 卡牌系统](#54-卡牌系统)
   - [5.5 事件系统](#55-事件系统)
   - [5.6 牌堆与牌区管理](#56-牌堆与牌区管理)
   - [5.7 身份与胜负规则](#57-身份与胜负规则)
   - [5.8 UI 架构（MVC）](#58-ui-架构mvc)
   - [5.9 网络复制](#59-网络复制)
6. [代码规范](#6-代码规范)
7. [开发工作流](#7-开发工作流)
8. [调试方法](#8-调试方法)
9. [核心 API 速查](#9-核心-api-速查)
10. [常见开发任务](#10-常见开发任务)
11. [已知限制与 M3 规划](#11-已知限制与-m3-规划)
12. [FAQ](#12-faq)

---

## 1. 项目概述

TKGamea 是一个基于 **Unreal Engine 5.6** 实现的**三国杀标准身份局**卡牌游戏。项目使用 C++ 编写核心逻辑，UI 层通过 UMG (Unreal Motion Graphics) Widget 蓝图实现，采用 **MVC 架构**。

### 已实现的游戏功能

- ✅ 4~10人标准身份局（主公/忠臣/反贼/内奸）
- ✅ 六阶段回合制（准备→判定→摸牌→出牌→弃牌→结束）
- ✅ 基本牌：杀、闪、桃、酒
- ✅ 非延时锦囊：顺手牵羊、过河拆桥、无中生有、南蛮入侵、万箭齐发、决斗、无懈可击、桃园结义
- ✅ 延时锦囊：乐不思蜀、兵粮寸断、闪电
- ✅ 装备牌：武器/防具/+1马/-1马/宝物
- ✅ 距离系统（武器范围/±1马/八卦阵判定）
- ✅ 三种响应模式（单目标/链式/逐人问询）
- ✅ 全套身份规则（分配/死亡/奖惩/胜负）

### 暂未实现

- ⏳ 武将技能系统（M3 阶段）
- ⏳ DataTable 驱动牌堆（当前使用 Debug 模式硬编码牌堆）
- ⏳ 五谷丰登、借刀杀人（pending M3）
- ⏳ 国战模式
- ⏳ 自动化测试

---

## 2. 环境搭建

### 2.1 必备软件

| 软件 | 版本要求 | 说明 |
|------|----------|------|
| Unreal Engine | **5.6.x** | 游戏引擎 |
| Visual Studio | **2022** (17.x) | C++ 编译环境 |
| Windows SDK | 10.0.19041+ | Windows 开发工具包 |
| Git | 任意版本 | 版本控制 |

### 2.2 获取项目

```powershell
git clone <仓库地址>
cd TKGame
```

### 2.3 生成项目文件

在项目根目录右键 `TKGamea.uproject`，选择 **"Generate Visual Studio project files"**，生成 `.sln` 解决方案。

### 2.4 打开并编译

打开 `TKGamea.sln`，选择 **DebugGame Editor** 或 **Development Editor** 配置，编译解决方案。

或者直接双击 `TKGamea.uproject` 启动编辑器（UE 会自动编译）。

### 2.5 目录结构确认

```
d:/Work/TKGame/
├── TKGamea.uproject           ← 项目文件（双击启动编辑器）
├── TKGamea.sln                ← Visual Studio 解决方案
├── Config/                    ← 配置文件目录
├── Content/                   ← 蓝图/资源目录
├── Source/                    ← C++ 源代码（本文档核心范围）
│   ├── TKGamea.Target.cs      ← 游戏构建目标
│   ├── TKGameaEditor.Target.cs← 编辑器构建目标
│   └── TKGamea/               ← 主模块
└── Doc/                       ← 设计文档
```

---

## 3. 项目结构总览

### 3.1 Source 目录详细结构

```
Source/
├── TKGamea.Target.cs                # 游戏构建目标 (Game)
├── TKGameaEditor.Target.cs          # 编辑器构建目标 (Editor)
└── TKGamea/                         # 主模块 (Module)
    ├── TKGamea.Build.cs             # 模块构建配置 + 依赖声明
    ├── TKGamea.h                    # 日志类别声明
    ├── TKGamea.cpp                  # 模块入口 (IMPLEMENT_PRIMARY_GAME_MODULE)
    ├── TKGameTypes.h                # ⭐ 核心类型定义（所有枚举/结构体）
    │
    ├── Core/                        # 🔴 框架层 — 游戏框架骨架
    │   ├── TKGameModeBase.h/.cpp           # 游戏模式（生命周期/开局流程）
    │   ├── TKGameStateBase.h/.cpp          # 游戏状态（回合组件/响应组件）
    │   ├── TKPlayerControllerBase.h/.cpp   # 玩家控制器（Server/Client RPC）
    │   ├── TKPlayerStateBase.h/.cpp        # 玩家状态（血/身份/手牌数，全部 Replicated）
    │   ├── TKTurnComponentBase.h/.cpp      # ⭐ 回合状态机（六阶段推进）
    │   ├── TKResponseComponent.h/.cpp      # ⭐ 响应系统核心（三种响应模式）
    │   ├── TKEventComponentBase.h/.cpp     # 事件系统（Tag → Handler 映射）
    │   ├── TKGameHUD.h/.cpp                # MVC Controller 层（数据采集→推送）
    │   └── TKCheatManager.h/.cpp           # 调试作弊管理器（12个控制台命令）
    │
    ├── Cards/                       # 🟠 牌堆层 — 牌的物理存在和位置
    │   ├── TKCardTypes.h                   # 调试牌堆模板条目
    │   ├── TKDeckComponent.h/.cpp          # ⭐ 牌堆组件（洗牌/摸牌/回收/判定）
    │   └── TKCardZoneComponent.h/.cpp      # 牌区组件（手牌/装备/判定区域管理）
    │
    ├── Game/                        # 🟡 卡牌层 — 卡牌具体行为
    │   ├── TKCardBase.h/.cpp               # ⭐ 卡牌抽象基类（模板方法模式）
    │   ├── TKCard_Basic.h/.cpp             # 基本牌（杀/闪/桃/酒）
    │   ├── TKCard_Trick.h/.cpp             # 非延时锦囊（顺手/过河/无中等）
    │   ├── TKCard_DelayedTrick.h/.cpp      # 延时锦囊（乐/兵/闪电）
    │   └── TKCard_Equipment.h/.cpp         # 装备牌
    │
    ├── Rules/                       # 🟢 规则层
    │   └── TKIdentityRuleComponent.h/.cpp  # 身份规则（分配/死亡/胜负）
    │
    └── Ui/                          # 🔵 视图层 — MVC View
        ├── TKGameUIData.h                  # 数据协议结构体（Model→View）
        ├── TKUserWidgetBase.h/.cpp         # Widget 基类
        ├── TKGameHudWidget.h/.cpp          # 主 HUD View (BlueprintNativeEvent)
        ├── TKDebugHudWidget.h/.cpp         # 调试 HUD（左上角信息面板，直接继承 UUserWidget）
        └── TKCardSlotWidget.h/.cpp         # 手牌卡牌 Widget
```

### 3.2 模块分层图

```
┌──────────────────────────────────────────────────────────────┐
│                     TKGamea Module                           │
├──────────────────────────────────────────────────────────────┤
│  构建依赖: Core, CoreUObject, Engine, InputCore,             │
│            EnhancedInput, GameplayTags, UMG, Slate, SlateCore│
├──────────┬──────────┬───────────┬───────────┬────────────────┤
│  Core/   │ Cards/   │  Game/    │  Rules/   │      Ui/       │
│ (框架层) │ (牌堆层) │ (卡牌层)  │ (规则层)  │   (视图层)     │
├──────────┼──────────┼───────────┼───────────┼────────────────┤
│GameMode  │DeckComp  │CardBase   │Identity   │GameHUD (Ctrl)  │
│GameState │CardZone  │Card_Basic │RuleComp   │GameHudWidget   │
│PlayerCtrl│CardTypes │Card_Trick │           │DebugHudWidget  │
│PlayerStat│          │Card_Delay │           │CardSlotWidget  │
│TurnComp  │          │Card_Equip │           │UIData          │
│ResponseC │          │           │           │UserWidgetBase  │
│EventComp │          │           │           │                │
│GameHUD   │          │           │           │                │
│CheatMgr  │          │           │           │                │
└──────────┴──────────┴───────────┴───────────┴────────────────┘
```

### 3.3 文件统计

| 类型 | 文件数 | 说明 |
|------|--------|------|
| `.h` (头文件) | 25 | 类声明 / USTRUCT 定义 |
| `.cpp` (实现) | 22 | C++ 实现代码 |
| `.cs` (构建脚本) | 3 | Unreal Build Tool 目标/模块 |
| 总计 | **50** | 不含配置文件 |

---

## 4. 核心架构

### 4.1 继承体系概览

```
Unreal Gameplay Framework 子类:

  AGameMode       → ATKGameModeBase        (游戏模式：生命周期、开局)
  AGameState      → ATKGameStateBase       (游戏状态：回合/响应/结果)
  APlayerController → ATKPlayerControllerBase (玩家控制器：Server RPC)
  APlayerState    → ATKPlayerStateBase     (玩家状态：血/身份/牌区)
  AHUD            → ATKGameHUD             (MVC Controller 层)
  UCheatManager   → UTKCheatManager        (调试命令)

卡牌继承体系:

  UObject → UTKCardBase (抽象基类，模板方法)
    ├── UTKCard_Basic         (杀/闪/桃/酒)
    ├── UTKCard_Trick          (非延时锦囊)
    ├── UTKCard_DelayedTrick   (延时锦囊)
    └── UTKCard_Equipment      (装备)

组件体系:

  UActorComponent → UTKTurnComponentBase    (回合状态机 → GameState)
  UActorComponent → UTKResponseComponent    (响应系统 → GameState)
  UActorComponent → UTKEventComponentBase   (事件系统 → PlayerController)
  UActorComponent → UTKDeckComponent        (牌堆管理 → GameMode)
  UActorComponent → UTKCardZoneComponent    (牌区管理 → PlayerState)
  UActorComponent → UTKIdentityRuleComponent (身份规则 → GameMode)

UI 体系:

  UUserWidget → UTKUserWidgetBase
    ├── UTKGameHudWidget      (主 HUD)
    └── UTKCardSlotWidget     (手牌卡片)
  UUserWidget → UTKDebugHudWidget (调试面板，直接继承 UUserWidget)
```

### 4.2 核心设计模式

| 模式 | 应用位置 | 说明 |
|------|----------|------|
| **模板方法** | `UTKCardBase::Use()` | 定义卡牌使用骨架流程，子类重写具体步骤 |
| **MVC** | HUD (Controller) + GameState/PlayerState (Model) + Widget (View) | 每帧 HUD 从 Model 采集数据推送 View |
| **策略模式** | `UTKIdentityRuleComponent` | 身份规则抽象，可切换不同模式 |
| **观察者模式** | `UTKEventComponentBase` | Tag → Handler 映射的事件发布/订阅 |
| **状态机** | `UTKTurnComponentBase` | 六阶段回合状态机 |

---

## 5. 关键系统详解

### 5.1 游戏启动流程

游戏启动通过 `ATKGameModeBase::StartIdentityGame()` 触发，完整流程如下：

```
StartIdentityGame()
│
├── ① 人数校验
│     └─ 检查 4 ≤ PlayerCount ≤ 10，否则拒绝
│
├── ② 初始化牌堆
│     └─ DeckComponent->InitDebugDeck() / InitFromDataTable()
│         ├── DataTable 模式：从 DeckDataTable 读取
│         └── Debug 模式：硬编码约 35 张测试用牌（小型调试牌堆）
│
├── ③ 分配身份
│     └─ IdentityRule->AssignIdentities()
│         ├── 主公固定座位 0
│         └── 其余通过随机偏移分配（忠臣/反贼/内奸）
│
├── ④ 发起始手牌
│     └─ DealInitialCards()
│         └─ 所有存活玩家各发 4 张（ModeConfig.InitialHandCount）
│
└── ⑤ 主公先手
      └─ SetLordFirstTurn()
          └─ TurnComponent->StartTurn(LordPlayerState)
```

### 5.2 回合状态机

`UTKTurnComponentBase` 是回合系统的核心，挂载在 `GameState` 上，管理六阶段推进：

```
┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
│ Prepare  │──→│  Judge   │──→│   Draw   │──→│   Play   │──→│ Discard  │──→│  Finish  │
│ 准备阶段 │   │ 判定阶段 │   │ 摸牌阶段 │   │ 出牌阶段 │   │ 弃牌阶段 │   │ 结束阶段 │
└──────────┘   └──────────┘   └──────────┘   └──────────┘   └──────────┘   └──────────┘
     │               │              │              │              │              │
     ▼               ▼              ▼              ▼              ▼              ▼
（无特殊逻辑，  （遍历判定区  （摸牌，被兵    （出牌，被乐  （手牌数>体力  （查找下一个
 自动推进）    延时锦囊结算）  则跳过）       则跳过）    值则自动弃牌） 存活玩家开始
                                                                       新回合）
```

**关键 API**:

```cpp
// 服务端推进回合
void StartTurn(APlayerState* Player);     // 开始新回合
void EnterPhase(ETKTurnPhase NewPhase);   // 进入指定阶段
void AdvancePhase();                       // 推进到下一阶段
void EndTurn();                            // 结束当前回合
void Stop();                               // 强制停止状态机（游戏结束时调用）

// 查询状态（Replicated）
APlayerState* GetCurrentPlayer() const;   // 当前回合玩家
ETKTurnPhase GetCurrentPhase() const;     // 当前阶段
int32 GetCurrentTurnNumber() const;       // 当前回合数

// 杀次数限制
int32 GetSlashUsedCount() const;          // 已用杀次数
int32 GetMaxSlashCount() const;           // 杀次数上限（默认=1）
bool CanUseSlash() const;                  // 是否还能出杀
void ConsumeSlash();                       // 消耗一次杀

// 跳过阶段标记
bool ShouldSkipDrawPhase() const;         // 兵粮寸断
bool ShouldSkipPlayPhase() const;         // 乐不思蜀

// 距离系统
int32 GetRawDistance(ATKPlayerStateBase* From, ATKPlayerStateBase* To);  // 原始座位距离
int32 GetWeaponRange(ATKPlayerStateBase* Player);                         // 武器攻击范围
bool CanReachTarget(ATKPlayerStateBase* Attacker, ATKPlayerStateBase* Defender); // 是否可攻击到
bool HasBaguaArmor(ATKPlayerStateBase* Player);                           // 是否有八卦阵

// 酒状态
bool IsWined() const;                      // 是否有酒状态
void SetWined(bool bVal);                  // 设置酒状态
```

### 5.3 响应系统

`UTKResponseComponent` 是整个游戏最复杂的子系统，挂载在 `GameState` 上，管理所有卡牌互动响应。

#### 三种响应模式

| 模式 | 枚举值 | 使用场景 | 流程 |
|------|--------|----------|------|
| **SingleTarget** | `ETKResponseType::SingleTarget` | 杀 → 闪 | 等待目标响应，超时自动结算 |
| **Chain** | `ETKResponseType::Chain` | 无懈可击链 | 全服广播，有人出无懈则 ChainDepth++，重新开链 |
| **Sequential** | `ETKResponseType::Sequential` | AOE / 濒死 / 决斗 | 逆时针逐人问询，每人超时后自动下一人 |

#### 响应请求生命周期

```
RequestResponse()  // 发起请求
    ↓
BroadcastRequestToClients()  // Chain→全服广播 / 其余→通知PrimaryTarget客户端
    ↓
┌─── ClientPromptResponse()  // 目标客户端弹出响应面板
│    ├── 玩家出牌 → 移除手牌 → SubmitResponse() → ResolveRequest()
│    └── 超时 → OnRequestTimeout() → ResolveRequest(Timeout)
│              (AOE 超时还会判定八卦阵及扣伤害)
    ↓
ContinueUseAfterResponse()  // 回调到卡牌对象，继续结算
    ↓
ProcessNextPending()  // 处理队列中下一个请求
```

#### 核心数据结构

```cpp
struct FResponseRequest {
    ETKResponseType Type;                // 响应类型
    FGameplayTag RequiredTag;            // 需要什么牌（如 Card.Effect.Dodge）
    UTKCardBase* SourceCard;             // 发起响应的卡牌
    APlayerState* Initiator;             // 发起者
    APlayerState* PrimaryTarget;         // 主目标
    TArray<APlayerState*> ResponderQueue; // Sequential 响应对列
    int32 CurrentResponderIndex;         // 当前等待的玩家索引
    TArray<APlayerState*> AlreadyResponded; // 已响应的玩家列表（Chain 防重复）
    ETKResponseResult Result;            // 当前结果
};
```

### 5.4 卡牌系统

#### 模板方法模式

`UTKCardBase` 使用**模板方法模式**定义卡牌使用骨架：

```
Use(User, Target)
│
├── ① PreUseCheck(User)
│     └─ 阶段/时机/距离校验
│
├── ② CanUse(User, Target)
│     └─ 目标合法性校验
│
├── ③ 如果需要响应 (NeedsResponse)
│     ├─ 构建 FResponseRequest
│     ├─ 提交到 ResponseComponent
│     └─ 等待回调 → ContinueUseAfterResponse()
│
├── ④ OnUse(User, Target)
│     └─ 子类具体效果（伤害/回血/摸牌/弃牌等）
│
└── ⑤ OnAfterUse(User, Target)
      ├─ 触发事件 (EventComponent)
      └─ 牌移入弃牌堆
```

#### 各类卡牌实现要点

**基本牌 (UTKCard_Basic)**:

| 牌型 | 关键行为 |
|------|----------|
| 杀 | 伤害结算 + 酒加成 + 杀次数限制 + 距离校验，不可对自己使用 |
| 闪 | 被动响应（不走主动 Use 流程），响应类型 = SingleTarget |
| 桃 | 仅可对自己使用，回复 1 点体力；响应阶段用于濒死求桃 |
| 酒 | 仅可对自己使用，体力>0 时设置酒状态（下张杀伤害+1），体力≤0 时回 1 血 |

**非延时锦囊 (UTKCard_Trick)**:

| 牌型 | 关键行为 |
|------|----------|
| 顺手牵羊 | 随机从目标手牌抽取一张 |
| 过河拆桥 | 随机弃置目标一张手牌 |
| 无中生有 | 摸两张牌 |
| 无懈可击 | Chain 模式抵消锦囊 |
| 决斗 | Duel 模式（双方轮流出杀） |
| 南蛮入侵 | Sequential 模式（全员出杀） |
| 万箭齐发 | Sequential 模式（全员出闪） |
| 桃园结义 | 全体回血1点 |

**延时锦囊 (UTKCard_DelayedTrick)**:
- 使用时不立即生效，放入目标判定区
- 在下个 Judge 阶段自动结算
- 乐不思蜀：判定非红桃 → 跳过 Play 阶段
- 兵粮寸断：判定非梅花 → 跳过 Draw 阶段
- 闪电：判定黑桃2~9 → 扣3血；否则传给下家

**装备牌 (UTKCard_Equipment)**:
- 使用后放入装备区（不进弃牌堆）
- 同槽位自动顶替旧装备

### 5.5 事件系统

`UTKEventComponentBase` 提供基于 GameplayTag 的事件发布/订阅机制。

```cpp
// 订阅事件
EventComponent->RegisterHandler(Tag, Handler);

// 发布事件（本地 + 通知 GameState）
EventComponent->PostEvent(Context);

// 本地处理
EventComponent->HandleLocalEvent(Context);
```

### 5.6 牌堆与牌区管理

#### 牌堆组件 (UTKDeckComponent)

挂载在 `GameMode` 上，管理：
- **洗牌**: Fisher-Yates 算法
- **摸牌**: 牌堆空时自动回收弃牌堆重新洗牌
- **弃牌回收**: 弃牌加入弃牌堆
- **判定系统**: `ExecuteJudgement()`, `IsJudgementEffective()` — 支持改判（预留）

#### 牌区组件 (UTKCardZoneComponent)

挂载在每个 `PlayerState` 上，管理玩家的三个区域：

| 区域 | 枚举 | 说明 |
|------|------|------|
| 手牌区 | `ETKCardZone::Hand` | 可打出/弃置的手牌 |
| 装备区 | `ETKCardZone::Equipment` | 武器/防具/±1马/宝物 |
| 判定区 | `ETKCardZone::Judgement` | 延时锦囊暂存区 |

### 5.7 身份与胜负规则

`UTKIdentityRuleComponent` 实现标准三国杀身份规则：

**身份分配表**:

| 人数 | 主公 | 忠臣 | 反贼 | 内奸 |
|------|------|------|------|------|
| 4 | 1 | 1 | 1 | 1 |
| 5 | 1 | 1 | 2 | 1 |
| 6 | 1 | 1 | 3 | 1 |
| 7 | 1 | 2 | 3 | 1 |
| 8 | 1 | 2 | 4 | 1 |
| 9 | 1 | 3 | 4 | 1 |
| 10 | 1 | 3 | 5 | 1 |

**胜负判定**（`ATKPlayerStateBase::CheckGameEnd()` 实际逻辑）:

```
主公死亡 → 反贼胜（收集所有存活反贼为胜者）
反贼全灭 → 主忠胜（收集所有存活主公和忠臣为胜者）
```

`UTKIdentityRuleComponent::EvaluateVictory()` 还提供了一个更精细但未被直接调用的判定：
```
主公死亡 + 仅内奸单独存活 → 内奸胜
主公死亡 + 有反贼存活 → 反贼胜
反贼和内奸全灭 + 主公存活 → 主忠胜
```

**死亡处理**: 遍历清空死亡玩家所有牌区（手牌/装备/判定）→ 弃牌堆回收（`CardZone->ClearAndDiscardAll(Deck)`）

**奖惩**: 反贼死亡时击杀者摸 3 张奖励牌；主公杀忠臣时弃置所有牌

### 5.8 UI 架构（MVC）

```
┌──────────────────────────────────────────────────────┐
│                      MVC 架构                        │
├───────────────┬──────────────┬───────────────────────┤
│     Model      │  Controller  │        View           │
│  (数据层)      │  (控制层)    │     (视图层)          │
├───────────────┼──────────────┼───────────────────────┤
│GameStateBase  │  TKGameHUD   │  TKGameHudWidget      │
│  - TurnComp   │              │  - RefreshDisplay()   │
│  - ResponseC  │  每帧 Tick:  │  - RefreshHandCards() │
│PlayerState    │  采集 Model  │  - ShowResponsePanel()│
│  - CardZone   │  数据 → 推送 │                        │
│  - Health     │  View        │  TKDebugHudWidget     │
│  - Identity   │              │  (直接继承UUserWidget,│
│               │              │   NativeTick 刷新)    │
│               │              │                        │
│  UIData 结构  │              │  TKCardSlotWidget      │
│  (数据协议)   │              │  (手牌卡片)            │
└───────────────┴──────────────┴───────────────────────┘
```

**数据协议** (`FTKGameUIData` / `FTKPlayerUIData` / `FTKResponseUIData`):
- 定义在 `Ui/TKGameUIData.h`
- 是 Model → View 的唯一数据通道
- View 不直接访问 Model，只能通过 HUD 推送的数据渲染

### 5.9 网络复制

项目采用 UE 原生网络复制，**服务器权威**模式：

| 组件/属性 | 复制方式 | 说明 |
|-----------|----------|------|
| `ATKPlayerStateBase::SeatIndex` | `Replicated` | 座位索引 |
| `ATKPlayerStateBase::bAlive` | `Replicated` | 存活状态 |
| `ATKPlayerStateBase::Identity` | `Replicated` | 身份（调试模式全部可见） |
| `ATKPlayerStateBase::CurrentHealth` | `Replicated` | 当前体力值 |
| `ATKPlayerStateBase::MaxHealth` | `Replicated` | 体力上限 |
| `ATKPlayerStateBase::HandCardCount` | `Replicated` | 手牌数（公开） |
| `ATKPlayerStateBase::bIdentityRevealed` | **不复制** | 身份揭示标记（仅服务器） |
| `UTKTurnComponentBase::CurrentPlayer` | `Replicated` | 当前回合玩家 |
| `UTKTurnComponentBase::CurrentPhase` | `Replicated` | 当前阶段 |
| `UTKTurnComponentBase::CurrentTurnNumber` | `Replicated` | 回合编号 |
| `ATKGameStateBase::GameResult` | `Replicated` | 游戏结果 |
| `UTKCardZoneComponent` 牌区数据 | `Replicated` | 手牌/装备/判定区（含客户端副本） |

**Server RPC**（客户端→服务器）:
- `ServerStartGame()`, `ServerAdvancePhase()`, `ServerUseCard()`, `ServerSubmitResponse()`

**Client RPC**（服务器→客户端）:
- `ClientPromptResponse()` — 通知客户端显示响应面板

---

## 6. 代码规范

### 6.1 命名约定

| 前缀 | 含义 | 示例 |
|------|------|------|
| `A` | Actor 子类 | `ATKGameModeBase` |
| `U` | UObject / Component 子类 | `UTKCardBase`, `UTKDeckComponent` |
| `F` | UStruct / 结构体 | `FTKCardInstance`, `FTKModeConfig` |
| `E` | Enum | `ETKTurnPhase`, `ETKCardZone` |
| `TK` | 项目前缀 (Three Kingdoms) | 所有自建类型 |

### 6.2 文件命名

```
格式: TK<类名>.h / TK<类名>.cpp
示例: TKCardBase.h / TKCardBase.cpp

子类后缀:
  _Basic      — 基本牌子类    (TKCard_Basic)
  _Trick      — 非延时锦囊    (TKCard_Trick)
  _DelayedTrick — 延时锦囊   (TKCard_DelayedTrick)
  _Equipment  — 装备牌子类    (TKCard_Equipment)

基类后缀:
  Base        — 可蓝图继承    (TKGameModeBase, TKCardBase)
```

### 6.3 编码风格

- **缩进**: 使用 **Tab** 字符
- **花括号**: 换行风格（Allman）
- **类成员**: 使用 `UPROPERTY()` 宏标记
- **日志**: 使用 `UE_LOG(LogTKGame, ...)` 输出
- **复制规则**: 使用 `GetLifetimeReplicatedProps()` 声明

### 6.4 注释规范

```cpp
/**
 * 类/函数主要说明（使用多行文档注释）
 *
 * 详细说明...
 * @param Name  参数说明
 * @return 返回值说明
 */
```

---

## 7. 开发工作流

### 7.1 添加新卡牌

1. 确定卡牌类型（基本/锦囊/延时锦囊/装备）
2. 在 `Game/` 目录下创建新的 `.h` 和 `.cpp` 文件
3. 继承对应的基类（`UTKCard_Basic`, `UTKCard_Trick`, `UTKCard_DelayedTrick`, `UTKCard_Equipment`）
4. 实现必要的虚函数：
   - `OnUse()` — 核心效果逻辑
   - `CanUse()` — 使用条件
   - `PreUseCheck()` — 阶段/时机校验
   - `NeedsResponse()` — 是否需要响应
   - `GetResponseType()` / `GetResponseRequiredTag()` — 响应配置
5. 在 `Config/DefaultGameplayTags.ini` 中添加对应 GameplayTag（如需要）
6. 在 `TKDeckComponent::InitializeDeck()` 中将新牌加入牌堆

### 7.2 添加新事件

1. 在 `Config/DefaultGameplayTags.ini` 中添加新的 GameplayTag
2. 在需要监听的地方调用 `EventComponent->RegisterHandler(Tag, Handler)`
3. 在触发事件的地方调用 `EventComponent->PostEvent(Context)`

### 7.3 修改回合逻辑

- 修改 `UTKTurnComponentBase::EnterPhase()` 中各阶段默认行为
- 子类可重写 `OnDrawPhase()`, `OnDiscardPhase()` 自定义逻辑
- 新增阶段需要修改 `ETKTurnPhase` 枚举和 `AdvancePhase()` 流转逻辑

### 7.4 添加新 UI 面板

1. 在 `Ui/` 目录创建 Widget 类（继承 `UTKUserWidgetBase`）
2. 定义 `FTKGameUIData` 相关数据协议
3. 在 `ATKGameHUD` 中添加对应的数据采集和推送逻辑
4. 在蓝图中实现 UI 表现

---

## 8. 调试方法

### 8.1 控制台命令（CheatManager）

所有命令在游戏运行时按 **`~`** 打开控制台输入：

| 命令 | 功能 |
|------|------|
| `StartDebugGame` | 启动调试模式游戏（自动开局） |
| `ShowDebugHud` | 显示调试 HUD 覆盖层 |
| `HideDebugHud` | 隐藏调试 HUD 覆盖层 |
| `ToggleDebugHud` | 切换调试 HUD 显示/隐藏 |
| `AdvancePhase` | 手动推进到下一回合阶段 |
| `ShowHand` | 打印当前玩家手牌信息 |
| `UseCard <CardIndex> [TargetIndex]` | 使用手牌中第 N 张卡（从0开始），可选指定目标索引 |
| `RespondCard <CardIndex>` | 用手牌中第 N 张卡作为响应牌 |
| `ShowResponseStatus` | 显示当前响应系统状态 |
| `ShowPlayerStatus` | 显示所有玩家状态 |
| `EquipCard <CardIndex>` | 装备手牌中第 N 张卡 |
| `ShowEquip` | 查看当前玩家装备区 |

### 8.2 调试 HUD

`UTKDebugHudWidget` 在屏幕左上角实时显示：
- 当前回合数 / 当前玩家 / 当前阶段
- 所有玩家状态表（座位/身份/体力/手牌数/存活）
- 牌堆信息
- 游戏结果
- 当前玩家手牌列表

### 8.3 日志输出

```cpp
// 使用项目统一日志类别
UE_LOG(LogTKGame, Log, TEXT("描述信息: %s, %d"), *SomeString, SomeInt);
UE_LOG(LogTKGame, Warning, TEXT("警告信息"));
UE_LOG(LogTKGame, Error, TEXT("错误信息"));
```

### 8.4 Blueprint Debugging

- 大部分关键函数都标记了 `BlueprintCallable`，可从蓝图调用测试
- 带有 `BlueprintNativeEvent` 的函数可在蓝图中覆盖

---

## 9. 核心 API 速查

### 9.1 GameMode

```cpp
// 开始游戏
bool ATKGameModeBase::StartIdentityGame();

// 结束游戏
void ATKGameModeBase::EndGame(ETKGameResult Result, 
                               const TArray<APlayerState*>& Winners);

// 获取组件
UTKDeckComponent* ATKGameModeBase::GetDeckComponent() const;
```

### 9.2 GameState

```cpp
// 获取回合组件
UTKTurnComponentBase* UTKTurnComponentBase::Get(const ATKGameStateBase* GS);

// 获取响应组件
UTKResponseComponent* UTKResponseComponent::Get(const ATKGameStateBase* GS);

// 游戏结果（Replicated）
FTKGameResultData ATKGameStateBase::GameResult;
```

### 9.3 PlayerController

```cpp
// 使用卡牌（仅接受卡牌参数，默认目标为自身）
void ATKPlayerControllerBase::ServerUseCard_Implementation(
    UTKCardBase* Card
);

// 提交响应
void ATKPlayerControllerBase::ServerSubmitResponse_Implementation(
    UTKCardBase* ResponseCard
);

// 推进阶段
void ATKPlayerControllerBase::ServerAdvancePhase_Implementation();
```

### 9.4 PlayerState

```cpp
// 玩家属性（全部 Replicated）
int32 ATKPlayerStateBase::SeatIndex;
bool ATKPlayerStateBase::bAlive;
ETKIdentity ATKPlayerStateBase::Identity;
int32 ATKPlayerStateBase::CurrentHealth;
int32 ATKPlayerStateBase::MaxHealth;
int32 ATKPlayerStateBase::HandCardCount;

// 牌区组件
UTKCardZoneComponent* ATKPlayerStateBase::GetCardZone() const;

// 状态与方法
bool ATKPlayerStateBase::IsAlive() const;
void ATKPlayerStateBase::SetIdentity(ETKIdentity NewIdentity);
bool ATKPlayerStateBase::ApplyDamage(int32 Damage, ATKPlayerStateBase* DamageSource = nullptr);
bool ATKPlayerStateBase::Heal(int32 HealAmount);
void ATKPlayerStateBase::TriggerDyingResponse();
```

### 9.5 TurnComponent

```cpp
// 推进（仅服务器）
void UTKTurnComponentBase::StartTurn(APlayerState* Player);
void UTKTurnComponentBase::EnterPhase(ETKTurnPhase NewPhase);
void UTKTurnComponentBase::AdvancePhase();
void UTKTurnComponentBase::EndTurn();
void UTKTurnComponentBase::Stop();

// 查询
APlayerState* GetCurrentPlayer() const;
ETKTurnPhase GetCurrentPhase() const;
int32 GetSlashUsedCount() const;
int32 GetMaxSlashCount() const;
bool CanUseSlash() const;
bool IsWined() const;

// 距离
int32 GetRawDistance(ATKPlayerStateBase* From, ATKPlayerStateBase* To) const;
bool CanReachTarget(ATKPlayerStateBase* Attacker, ATKPlayerStateBase* Defender) const;
bool HasBaguaArmor(ATKPlayerStateBase* Player) const;
```

### 9.6 ResponseComponent

```cpp
// 发起响应
void UTKResponseComponent::RequestResponse(
    const FResponseRequest& Request, 
    FOnResponseResolved OnResolved
);

// 提交响应
bool UTKResponseComponent::SubmitResponse(
    ATKPlayerStateBase* Responder, 
    UTKCardBase* ResponseCard
);

// 查询
bool IsWaitingForResponse() const;
TArray<APlayerState*> GetAlivePlayers() const;
```

### 9.7 DeckComponent

```cpp
// 洗牌
void UTKDeckComponent::Shuffle();

// 摸单张牌
UTKCardBase* UTKDeckComponent::DrawCard();

// 摸多张牌
TArray<UTKCardBase*> UTKDeckComponent::DrawMultipleCards(int32 Count);

// 弃牌回收（接受 UTKCardBase*）
void UTKDeckComponent::DiscardCard(UTKCardBase* Card);

// 判定（输出引用方式返回花色/点数/判定牌）
bool UTKDeckComponent::ExecuteJudgement(
    const FGameplayTag& JudgeTag,
    ETKCardSuit& OutSuit,
    int32& OutRank,
    UTKCardBase*& OutCard
);

// 判定生效条件检查（静态方法）
static bool UTKDeckComponent::IsJudgementEffective(
    const FGameplayTag& JudgeTag,
    ETKCardSuit Suit,
    int32 Rank
);
```

### 9.8 卡牌基类

```cpp
// 模板方法（入口）
virtual bool UTKCardBase::Use(
    ATKPlayerControllerBase* User, 
    ATKPlayerStateBase* Target
);

// 子类重写方法
virtual bool PreUseCheck(ATKPlayerControllerBase* User) const;
virtual bool CanUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) const;
virtual bool NeedsResponse() const;
virtual ETKResponseType GetResponseType() const;
virtual FGameplayTag GetResponseRequiredTag() const;
virtual void OnUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target);
virtual void OnAfterUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target);
```

### 9.9 事件系统

```cpp
// 注册事件处理器
void UTKEventComponentBase::RegisterHandler(
    const FGameplayTag& EventTag, 
    FOnGameEvent Handler
);

// 发布事件
void UTKEventComponentBase::PostEvent(FEventContext& Context);

// 本地处理事件
void UTKEventComponentBase::HandleLocalEvent(const FEventContext& Context);
```

---

## 10. 常见开发任务

### 10.1 添加新身份模式（如5人局变体）

1. 在 `TKIdentityRuleComponent` 中添加新的身份分配表
2. 修改 `StartIdentityGame()` 支持新模式
3. 更新 `FTKModeConfig` 增加模式选项

### 10.2 添加新武器

1. 在 `UTKCard_Equipment` 中添加武器类型
2. 设置武器的攻击范围属性
3. 在 `UTKTurnComponentBase::GetWeaponRange()` 中支持新武器

### 10.3 添加 GameplayTag

在 `Config/DefaultGameplayTags.ini` 中添加：

```ini
+GameplayTagList=(Tag="Card.Effect.YourNewEffect",DevComment="新效果说明")
```

### 10.4 在蓝图中创建 UI

1. 在 Content Browser 右键创建 Widget Blueprint
2. 选择 `UTKUserWidgetBase` 作为父类
3. 绑定蓝图事件（如 `OnGameDataUpdated`, `OnHandCardsUpdated`）
4. 在 `ATKGameHUD` 中管理该 Widget 的生命周期

---

## 11. 已知限制与 M3 规划

### 当前限制

| 类别 | 限制说明 |
|------|----------|
| 牌堆 | 使用 Debug 硬编码数据，尚未接入 DataTable |
| 武将 | 武将技能系统标注为 M3 启用 |
| 测试 | 无自动化测试，只依赖 CheatManager 手工调试 |
| UI | 部分 UI 功能在蓝图层尚未完全实现 |
| 卡牌 | 五谷丰登、借刀杀人标注为 "pending M3" |
| 模式 | 仅实现身份局，国战模式为预留 |

### M3 阶段规划

- 武将技能框架
- DataTable 驱动牌堆
- 五谷丰登、借刀杀人
- 更多 UI 完善

---

## 12. FAQ

### Q: 为什么 `FTKCardInstance` 不是 UObject？
A: 卡牌实例在网络间大量传输，使用 `USTRUCT` 轻量复制效率更高。`FTKCardInstance` 是对 `UTKCardBase` UObject 的轻量引用，客户端通过 `CardDefId` 找到对应对象。

### Q: 为什么 `ResponseComponent` 在 GameState 而不在 GameMode？
A: GameMode 仅存在于服务器，GameState 会复制到所有客户端。响应状态需要在客户端可见（用于 UI 显示），因此挂载在 GameState 上。

### Q: 如何区分服务器端逻辑和客户端逻辑？
A: 使用 `HasAuthority()` 判断是否为服务器。所有回合推进、卡牌结算、规则判定都在服务器执行，客户端只需等待 Replicated 属性更新。

### Q: 牌的 UObject 存在哪里？销毁时机？
A: `UTKCardBase` 子类 UObject 在需要时创建（从牌堆摸出时），使用完毕后（弃牌/判定/使用完毕）标记回收或销毁。`FTKCardInstance` 作为轻量副本用于网络传输和 UI 显示。

### Q: 如何导出蓝图可调用的函数？
A: 在函数声明前添加 `UFUNCTION(BlueprintCallable)` 宏。查询函数可使用 `BlueprintPure`。需要蓝图重写的使用 `BlueprintNativeEvent`。

---

## 参考文档

项目 `Doc/` 目录下包含以下设计文档，建议阅读：

| 文档 | 说明 |
|------|------|
| `Sanguosha_Development_Requirements.md` | 三国杀开发需求文档 |
| `Sanguosha_Code_Gap_Analysis.md` | 代码差距分析 |
| `Logic_Consistency_Review_Report.md` | 逻辑一致性审查报告 |
| `P0_Blocking_Defects_Solution.md` | P0 阻塞缺陷解决方案 |
| `UI_Framework_Implementation_Plan.md` | UI 框架实现计划 |
| `Phase2_UI_Implementation_Plan.md` | 第二阶段 UI 实现计划 |
| `MVC_UI_Design.md` | MVC UI 设计文档 |
| `追踪卡牌数据从诞生到显示的全链路.md` | 卡牌数据全链路追踪 |

---

> 本文档基于 `d:/Work/TKGame/Source/` 下全部 50 个源代码文件分析并逐文件校验生成。  
> 如有疑问，请查阅源码或参考 Doc/ 目录下的设计文档。
