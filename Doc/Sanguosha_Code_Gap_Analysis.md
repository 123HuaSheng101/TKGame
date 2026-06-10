# TKGame 三国杀现有代码缺口与 M2 设计规划

> 分析日期：2026-06-10
> 分析范围：`Source/TKGamea` 当前真实代码状态
> 版本状态：M1（身份基础模式最小闭环）已完成，以下为 M2 规划

---

## 1. 当前代码状态（M1 已完成工作）

当前工程已完成身份基础模式的最小闭环，可从开局走到胜负。

### 1.1 已完成模块清单

| 模块 | 状态 | 说明 |
| --- | --- | --- |
| **TKGameTypes.h** | ✅ 完成 | 定义 `ETKGameModeType`、`ETKTurnPhase`、`ETKIdentity`、`ETKCardZone`、`ETKGameResult`、`ETKCardSuit`、`ETKCardType` 枚举；`FTKModeConfig`、`FTKCardDef`、`FTKCardInstance`、`FTKGameResultData`、`FEventContext` 结构体 |
| **ATKGameModeBase** | ✅ 完成 | `StartIdentityGame()` 完整流程：人数校验→初始化牌堆→分配身份→发初始手牌→主公先手→设置 InProgress；`EndGame()` 结算 |
| **ATKGameStateBase** | ✅ 完成 | `GameResult` Replicated，持有 `TurnComponent` |
| **ATKPlayerStateBase** | ✅ 完成 | `SeatIndex`、`bAlive`、`Identity`、`CurrentHealth`、`MaxHealth`、`HandCardCount` 均复制；`TakeDamage()`、`Heal()`、`SetIdentity()` |
| **ATKPlayerControllerBase** | ✅ 完成 | `ServerStartGame()` / `ServerAdvancePhase()` RPC；创建 `EventComponent`；挂载 `UTKCheatManager` |
| **UTKTurnComponentBase** | ✅ 完成 | 6 阶段状态机（Prepare→Judge→Draw→Play→Discard→Finish→EndTurn）；`CurrentPlayer`/`CurrentPhase`/`CurrentTurnNumber` 复制；`AdvancePhase()`/`EndTurn()` 完整 |
| **UTKDeckComponent** | ✅ 完成 | `InitDebugDeck()`、`Shuffle()`（Fisher-Yates）、`DrawCard()`、`DrawMultipleCards()`、`DiscardCard()`；牌堆空时自动从弃牌堆洗回 |
| **UTKCardZoneComponent** | ✅ 完成 | `AddCardToHand()`/`RemoveCardFromHand()`/`GetHandCards()`、`AddToEquipment()`/`AddToJudgement()`、`ClearAllZones()` |
| **UTKIdentityRuleComponent** | ✅ 完成 | 4-10 人身份分配表、随机偏移分配、`EvaluateVictory()`（主忠/反贼/内奸三胜）、`ResolveDeath()` 清牌+胜负判定 |
| **UTKCardBase** | ✅ 完成 | `CardDefId`/`CardName`/`CardType`/`Suit`/`Rank`/`ZoneType`；`ActiveCard()` 为 M1 空实现 |
| **UTKCheatManager** | ✅ 完成 | `StartDebugGame`、`ShowDebugHud`/`HideDebugHud`/`ToggleDebugHud`、`AdvancePhase` 调试命令 |
| **UTKDebugHudWidget** | ✅ 完成 | 每帧 Tick 显示回合信息、阶段、玩家状态表、牌堆信息、游戏结果 |
| **TKCardTypes.h** | ✅ 完成 | `FTKDebugCardEntry`、`TKCardZoneComponent`/`TKDeckComponent` |

---

## 2. M1 已实现但留有缺口的模块（需 M2 完善）

以下模块虽然 M1 已实现，但在功能完整度上仍存在缺口，需要 M2 迭代：

| 模块 | 当前问题 | M2 改进目标 |
| --- | --- | --- |
| **UTKEventComponentBase** | `PostEvent()` 为空实现，事件系统无实质功能 | 实现完整的事件总线：订阅、派发、监听 |
| **UTKCardBase::ActiveCard()** | 空实现，卡牌效果无处执行 | 实现卡牌效果执行框架 + 具体卡牌（杀、桃、酒等） |
| **UTKTurnComponentBase** | Draw 阶段空实现（默认摸 2 在外部完成），Discard 阶段空实现 | Draw 阶段集成摸牌逻辑；Discard 阶段集成弃牌校验 |
| **胜负判定触发** | `ResolveDeath()` 直接调用 `EvaluateVictory()` 再通知 GameMode | 通过事件系统解耦：死亡→事件→规则判定→GameMode 结束 |
| **PlayerState Health 复制** | `CurrentHealth` 无 `OnRep`，客户端无法感知血量变化 | 添加 `OnRep_CurrentHealth` 回调 |

---

## 3. M2 核心需求（基于当前缺口）

### 3.1 P0：事件系统（最大短板）

**当前问题**：`UTKEventComponentBase::PostEvent()` 是空函数，没有任何地方调用。所有交互（摸牌、扣血、死亡判定等）都是直接方法调用，模块间耦合度高。

**M2 需求**：

| 需求项 | 说明 |
| --- | --- |
| **事件订阅/派发机制** | 基于 `GameplayTags` 实现事件总线；提供 `SubscribeEvent(Tag, Callback)` / `PostEvent(Context)` 接口 |
| **强类型事件上下文** | 为伤害、用牌、濒死、死亡、回合切换等核心事件建立专用 struct |
| **事件监听** | `UObject` 可注册监听特定 Tag 事件，支持过滤条件 |
| **通用 Tag 定义** | `Event.Game.Started`、`Event.Phase.Begin`、`Event.Card.Used`、`Event.Player.Damaged`、`Event.Player.Dying`、`Event.Player.Death`、`Event.Game.Ended` |

**设计约束**：
- 保留 `FEventContext` 的通用性，同时为高频事件增加强类型 payload
- 事件系统应支持技能注册和触发（为 M3 武将技能框架铺路）

### 3.2 P0：卡牌效果框架（ActiveCard 补齐）

**当前问题**：`UTKCardBase::ActiveCard()` 为空，卡牌打出后无效果。所有卡牌（杀、桃、闪、酒、锦囊、装备）的效果如何执行不明确。

**M2 需求**：

| 需求项 | 说明 |
| --- | --- |
| **卡牌效果抽象** | 定义 `UTKCardEffect` 基类，每种具体效果继承实现 |
| **数据驱动** | 卡牌效果通过 `DataTable` 配置，关联 Effect Class |
| **基础卡牌效果** | 实现【杀】（指定目标造成伤害）、【闪】（抵消杀）、【桃】（回复 1 体力）、【酒】（伤害+1 / 濒死回复） |
| **卡牌使用流程** | 选定目标 → 合法性校验 → 使用事件 → 执行效果 → 进入弃牌堆 |
| **非延时锦囊** | 实现【过河拆桥】、【顺手牵羊】、【无中生有】、【决斗】、【南蛮入侵】、【万箭齐发】、【桃园结义】、【五谷丰登】等 |
| **延时锦囊** | 实现【乐不思蜀】（判定阶段判定，非红桃则跳过出牌）、【兵粮寸断】（非梅花则跳过摸牌）、【闪电】（黑桃 2-9 受 3 点雷电伤害） |
| **装备牌** | 实现武器（攻击范围、特效）、防具（【八卦阵】判定、【仁王盾】黑杀无效）、+1/-1 马 |

### 3.3 P0：响应窗口系统

**当前问题**：没有"请求-响应"机制来实现"濒死求桃"、"出闪"、"无懈可击"等需要其他玩家响应的场景。

**M2 需求**：

| 需求项 | 说明 |
| --- | --- |
| **响应窗口框架** | 定义 `UTKResponseComponent`（或集成到 EventComponent），管理"等待响应→超时/确认→继续流程"状态机 |
| **濒死求桃** | 体力降至 0 → 进入濒死状态 → 按顺序（从当前回合角色开始逆时针）询问是否使用【桃】/【酒】→ 无人响应则死亡 |
| **杀/闪响应** | 使用【杀】指定目标 → 目标选择是否出【闪】→ 不出闪则受伤害 |
| **决斗响应** | 发起者出【杀】→ 目标出【杀】→...→ 无法出【杀】者受伤害 |
| **无懈可击** | 锦囊使用时，按顺序询问是否使用【无懈可击】抵消 |
| **超时机制** | 每个响应设超时时限（AI/调试模式自动选择，真人模式等待输入） |
| **消息顺序** | 响应过程应有清晰的消息队列，避免多个响应同时触发 |

### 3.4 P1：回合阶段补齐

**当前问题**：Draw 阶段和 Discard 阶段目前为空实现。

**M2 需求**：

| 需求项 | 说明 |
| --- | --- |
| **Draw 阶段** | 摸 2 张牌（默认），受【兵粮寸断】等影响 |
| **Play 阶段出牌限制** | 每回合仅可使用 1 张【杀】（受【诸葛连弩】等影响），需要 `UTKTurnComponentBase` 记录阶段出杀计数 |
| **Discard 阶段** | 检查手牌数是否超过当前体力值 → 必须弃至体力值上限、弃牌校验 |
| **Judge 阶段** | 判定区延时锦囊的判定执行（按后进先出顺序） |

### 3.5 P1：伤害与濒死系统完善

**当前问题**：`TakeDamage()` 和 `Heal()` 已实现但过于简单，缺乏伤害类型、伤害事件、濒死流程。

**M2 需求**：

| 需求项 | 说明 |
| --- | --- |
| **伤害类型** | 普通伤害、火焰伤害、雷电伤害；对应不同免疫/弱点 |
| **伤害事件** | 造成伤害前（可被【酒】增幅）、造成伤害后（触发技能如【反馈】、【刚烈】） |
| **伤害计算** | 基础伤害值 ± 伤害增减（如【酒】+1） |
| **濒死完整流程** | 进入濒死 → 求桃/酒 → 无人响应 → 死亡 |
| **伤害来源追溯** | 记录伤害来源（用于奖惩、技能触发） |
| **回复流程** | 使用【桃】/【桃园结义】等回复时，触发 OnHeal 事件 |

### 3.6 P1：死亡奖惩

**当前问题**：`ResolveDeath()` 已实现但未整合事件系统。

**M2 需求**：

| 需求项 | 说明 |
| --- | --- |
| **杀反贼摸 3** | 反贼死亡时，击杀者摸 3 张牌 |
| **主公杀忠臣** | 主公杀死忠臣时，主公弃置所有手牌和装备区/判定区牌 |
| **身份亮明** | 角色死亡后公开身份 |
| **死亡移交** | 死亡角色弃置所有区域的牌，进入 OutOfGame |

### 3.7 P1：PlayerState Health 复制完善

**当前问题**：`CurrentHealth` 和 `MaxHealth` 缺少 `OnRep` 回调，客户端无法实时感知血量变化。

**M2 需求**：

```cpp
UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
int32 CurrentHealth;

UFUNCTION()
void OnRep_CurrentHealth();
```

### 3.8 P1：调试与测试完善

| 需求项 | 说明 |
| --- | --- |
| **CheatManager 扩展** | 增加 `DrawCard`、`KillPlayer`、`SetHealth`、`SetIdentity` 等调试命令 |
| **自动化测试入口** | GameMode 增加 `DebugStartGame(FixedIdentityArray, FixedDeck)` 支持固定场景 |
| **日志输出** | 每个阶段切换、事件触发、伤害、死亡输出结构化日志，便于复盘 |
| **测试用例（ID-001 ~ ID-010）** | 见第 6 章测试清单 |

### 3.9 P2：技能框架预留（为 M3 铺路）

**当前问题**：完全没有技能/能力的抽象层。

**M2 需求**（仅框架，不实现具体武将技能）：

| 需求项 | 说明 |
| --- | --- |
| **技能基类** | 定义 `UTKSkillBase`，包含技能名、描述、触发条件（GameplayTag）、触发时机（主动/被动） |
| **技能注册** | 武将可关联多个技能，技能可注册到事件系统监听特定 Tag |
| **技能触发** | 事件触发时检查满足条件的技能，按优先级执行 |
| **技能类型** | 主动技能（玩家选择发动）、被动技能（条件满足自动触发）、锁定技（强制触发） |

---

## 4. 架构改进建议

### 4.1 事件系统与响应窗口架构

```
┌──────────────────────────────────────────────────────────┐
│                    UTKEventComponent                     │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │  Event Bus    │  │  Listener    │  │  Response     │  │
│  │  (Tag分发)     │  │  Registry    │  │  Window Mgr   │  │
│  └──────────────┘  └──────────────┘  └───────────────┘  │
│         │                  │                  │         │
│  PostEvent()         SubscribeEvent()    StartResponse() │
│  OnEvent()           UnsubscribeEvent()  CancelResponse()│
└──────────────────────────────────────────────────────────┘
```

**建议归属**：EventComponent 挂在 `GameState` 上（全局事件总线），而非挂在 PlayerController 上。原因：
- 事件是全游戏范围的，不归属某个玩家
- RuleComponent、CardEffect、Skill 等非 PlayerController 对象也需要访问事件
- 当前挂在 PlayerController 上会导致其他对象访问事件时需要绕路

### 4.2 卡牌效果架构

```
UTKCardEffect (UObject 基类)
├── UTKSlashEffect      —— 【杀】：对目标造成 1 点伤害
├── UTKDodgeEffect      —— 【闪】：抵消一次【杀】
├── UTKPeachEffect      —— 【桃】：回复 1 点体力 / 濒死救援
├── UTKLiquorEffect     —— 【酒】：本回合伤害+1 / 濒死救回复 2
├── UTKDismantleEffect  —— 【过河拆桥】：弃目标一张牌
├── UTKSnatchEffect     —— 【顺手牵羊】：获得目标一张牌
├── UTKNullifyEffect    —— 【无懈可击】：抵消一张锦囊
├── UTKDuelEffect       —— 【决斗】：交替出杀
├── UTKStrikeEffect     —— 【南蛮入侵】：所有其他角色出杀
├── UTKVolleyEffect     —— 【万箭齐发】：所有其他角色出闪
├── UTKPeachGardenEffect—— 【桃园结义】：所有角色回复满
├── UTKHogtieEffect     —— 【乐不思蜀】：延时锦囊
├── UTKFamineEffect     —— 【兵粮寸断】：延时锦囊
├── UTKLightningEffect  —— 【闪电】：延时锦囊
```

**设计思路**：
- `UTKCardDef`（数据表行）绑定 `TSubclassOf<UTKCardEffect>`，运行时实例化
- Effect 通过 `Execute(Context)` 执行效果，Context 包含使用者、目标、卡牌等
- Effect 内部可以通过事件系统派发子事件

### 4.3 响应窗口架构

```
UTKResponseWindow (UObject)
├── ResponseId / 发起者 / 当前响应者 / 响应队列
├── Start()      —— 开启窗口，开始按顺序询问
├── AddResponder(PlayerController)  —— 加入响应队列
├── SubmitResponse(bool bAccept, UCardBase* Card) —— 提交响应
├── OnTimeout()  —— 超时处理
└── OnComplete() —— 所有响应完成回调
```

### 4.4 建议目录结构演进（M2 后）

```
Source/TKGamea/
  Core/
    TKGameModeBase.*
    TKGameStateBase.*
    TKPlayerStateBase.*
    TKPlayerControllerBase.*
    TKTurnComponentBase.*
    TKEventComponentBase.*        ← M2: 重构为完整事件系统
    TKResponseComponent.*         ← M2: 新增响应窗口
    TKCheatManager.*
  Cards/
    TKCardTypes.h
    TKCardBase.*
    TKCardEffectBase.*            ← M2: 新增卡牌效果基类
    TKCardSlashEffect.*           ← M2: 新增具体效果子类
    TKCardPeachEffect.*
    TKCardDodgeEffect.*
    TKCardTrickEffects.*
    TKCardEquipmentEffects.*
    TKCardZoneComponent.*
    TKDeckComponent.*
  Rules/
    TKIdentityRuleComponent.*     ← M1 已完成，M2 整合事件系统
    TKNationalWarRuleComponent.*
    TKGameRuleBase.*              ← M2: 新增规则基类
  Skills/
    TKSkillBase.*                 ← M2: 新增技能框架（预留）
    TKSkillTypes.h
  Ui/
    TKUserWidgetBase.*
    TKDebugHudWidget.*
    TKGameHudWidget.*             ← M2: 新增正式游戏 UI
```

---

## 5. M2 实现优先级

| 优先级 | 模块 | 依赖 | 工作量估计 |
| --- | --- | --- | --- |
| **P0** | 事件系统（Event Bus + 强类型 Context） | 无 | 3-4 天 |
| **P0** | 卡牌效果框架 + 基础卡牌杀/闪/桃/酒 | 事件系统 | 5-7 天 |
| **P0** | 响应窗口系统（濒死求桃/杀闪响应/无懈/决斗） | 事件系统 | 4-5 天 |
| **P1** | 回合阶段补齐（Draw/Discard/Judge 逻辑） | TurnComponent 已有 | 2-3 天 |
| **P1** | 伤害系统完善 + PlayerState OnRep | 事件系统 | 1-2 天 |
| **P1** | 非延时锦囊（拆/顺/决斗/南蛮/万箭/无中生有等） | 卡牌效果框架+响应窗口 | 4-5 天 |
| **P1** | 延时锦囊（乐/兵/闪电） | Judge 阶段 + 卡牌效果框架 | 2-3 天 |
| **P1** | 死亡奖惩 + 事件系统整合 | 事件系统 | 1-2 天 |
| **P1** | 调试命令扩展 + 自动化测试 | CheatManager 已有 | 2-3 天 |
| **P1** | UI 初步（手牌、血量、阶段指示） | PlayerState OnRep | 3-4 天 |
| **P2** | 技能框架预留 | 事件系统 | 2-3 天 |
| **P2** | 装备牌（武器/防具/马） | 卡牌效果框架 | 3-4 天 |

**建议 M2 第 1 周**：事件系统 + 响应窗口 + 基础卡牌（杀/闪/桃/酒）
**建议 M2 第 2 周**：非延时锦囊 + 回合阶段补齐 + 伤害完善
**建议 M2 第 3 周**：延时锦囊 + 装备 + 死亡奖惩整合 + 调试完善
**建议 M2 第 4 周**：技能框架 + UI + 测试

---

## 6. 测试清单（身份模式，ID-001 ~ ID-010）

| 编号 | 测试目标 | 状态 | 建议验证点 |
| --- | --- | --- | --- |
| ID-001 | 开局身份分配 | ✅ M1 已实现 | 主公公开，其他身份仅本人可见 |
| ID-002 | 主公死亡且内奸唯一存活 | ✅ M1 已实现 | 内奸胜利 |
| ID-003 | 主公死亡且反贼存活 | ✅ M1 已实现 | 反贼胜利 |
| ID-004 | 反贼和内奸全部死亡 | ✅ M1 已实现 | 主忠胜利 |
| ID-005 | 杀死反贼 | 🔄 M2 实现击杀者摸 3 张 | 击杀者摸 3 张 |
| ID-006 | 主公杀死忠臣 | 🔄 M2 实现弃牌惩罚 | 主公弃置所有牌 |
| ID-007 | 出牌阶段连续使用两张【杀】 | 🔄 M2 实现次数限制 | 第二张被拒绝 |
| ID-008 | 判定区已有同名牌 | 🔄 M2 实现判定位限制 | 同名牌不能再次放入 |
| ID-009 | 弃牌阶段手牌超过体力 | 🔄 M2 实现弃牌校验 | 必须弃至当前体力 |
| ID-010 | 体力降至 0 后使用【桃】 | 🔄 M2 实现濒死响应 | 回复到至少 1 并退出濒死 |

---

## 7. M2 关键技术决策

### 7.1 EventComponent 归属

| 方案 | 挂载位置 | 优缺点 |
| --- | --- | --- |
| **方案 A（建议）** | 挂在 GameState 上作为全局事件总线 | ✅ 所有模块可访问；❌ 客户端需要复制访问 |
| **方案 B（当前）** | 挂在 PlayerController 上 | ❌ 非 PC 对象无法访问；⚠️ 需要 Get() 工具方法 |
| **方案 C** | 全局 Singleton Subsystem | ✅ 全局可达；❌ 网络复制需要额外设计 |

**建议**：M2 将 EventComponent 从 `PlayerController` 迁移到 `GameState`，并保留 `Get()` 工具方法在 GameState 上。

### 7.2 响应窗口归属

| 选项 | 说明 |
| --- | --- |
| 合并到 EventComponent | 响应是事件系统的子功能 |
| 独立 UTKResponseComponent | 职责分离，但挂在 GameState 还是 GM 需考虑 |

**建议**：先在 EventComponent 内实现响应窗口功能，后续复杂度上升时再拆为独立组件。

### 7.3 卡牌效果执行链路

```
玩家使用卡牌
  → 合法性校验（距离、目标、出杀次数等）
  → PostEvent(Event.Card.Using)   ← 技能/装备可拦截/修改
  → CardEffect->PreCheck()        ← 规则校验（如无懈可击）
  → 响应窗口（无懈可击）
  → CardEffect->Execute()         ← 执行效果
  → PostEvent(Event.Card.Used)    ← 技能/装备可响应
  → 卡牌移入弃牌堆
```

---

## 8. 当前代码中的即时风险

> 以下风险在 M1 中已标记，部分已修复。

| 风险 | 状态 | 当前情况 |
| --- | --- | --- |
| `UTKEventComponentBase::Get()` 断言风险 | ✅ **已修复** | `ATKPlayerControllerBase` 已在构造函数中创建 EventComponent |
| `UTKCardBase::ActiveCard()` 空实现 | ⚠️ **M1 可接受** | 仅有声明和空定义，M2 补齐 |
| `TurnComponent` 访问权限过严 | ⚠️ **M1 可接受** | 当前 private+friend，M2 建议增加 Public Getter |
| `FEventContext` 参数类型松散 | ⚠️ **M2 要解决** | M2 增加强类型事件上下文 |
| `PlayerState Health` 无 OnRep | ⚠️ **M2 要解决** | M2 增加 `OnRep_CurrentHealth` |
| 事件系统挂在 PlayerController 上 | ⚠️ **M2 要解决** | M2 迁移到 GameState 或增加全局访问路径 |
