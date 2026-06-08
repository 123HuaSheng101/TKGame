# TKGame 三国杀现有代码缺口与设计建议

> 分析日期：2026-06-08  
> 依据文档：`Sanguosha_Development_Requirements.md`  
> 分析范围：`Source/TKGamea`、`Config`、`README.md`  
> 结论摘要：当前工程已经有 Unreal C++ 基础类、回合组件壳子、事件上下文壳子和卡牌 UObject 壳子，但尚未形成三国杀任一模式的可运行规则闭环。建议先完成身份基础模式 M1/M2，再扩展武将框架与国战。

## 1. 当前代码状态

| 模块 | 当前实现 | 代码位置 | 判断 |
| --- | --- | --- | --- |
| 对局生命周期 | `ATKGameModeBase` 继承 `AGameMode`，构造函数和 `OnMatchStateSet()` 为空 | `Source/TKGamea/Core/TKGameModeBase.h:14`、`Source/TKGamea/Core/TKGameModeBase.cpp:13` | 只有入口，没有开始对局、分配身份、结束结算 |
| 对局状态 | `ATKGameStateBase` 只创建 `TurnComponent` | `Source/TKGamea/Core/TKGameStateBase.h:14`、`Source/TKGamea/Core/TKGameStateBase.cpp:10` | 没有玩家状态、模式配置、牌堆、胜负状态 |
| 回合组件 | `UTKTurnComponentBase` 可复制，提供 `Get(GameState)` | `Source/TKGamea/Core/TKTurnComponentBase.h:12`、`Source/TKGamea/Core/TKTurnComponentBase.cpp:15` | 没有当前玩家、阶段枚举、推进逻辑 |
| 事件组件 | `UTKEventComponentBase` 可复制，`PostEvent()` 为空，`Get()` 从 PlayerController 查组件 | `Source/TKGamea/Core/TKEventComponentBase.h:18`、`Source/TKGamea/Core/TKEventComponentBase.cpp:15` | 有事件雏形，但没有派发、订阅、响应窗口；且 PlayerController 未创建该组件 |
| 事件上下文 | `FEventContext` 包含 GameplayTag、发起者、目标、参数数组 | `Source/TKGamea/TKGameTypes.h:7` | 可作为事件原型，但类型过于松散，需要强类型规则上下文 |
| 玩家控制器 | `ATKPlayerControllerBase` 为空 | `Source/TKGamea/Core/TKPlayerControllerBase.h:14` | 没有输入、私有视角、手牌交互、响应 RPC |
| 卡牌 | `UTKCardBase` 只有 `ActiveCard()` 声明，cpp 未定义 | `Source/TKGamea/Game/TKCardBase.h:13`、`Source/TKGamea/Game/TKCardBase.cpp:4` | 没有卡牌实例字段、配置、目标、效果和牌区流转；`ActiveCard()` 若被调用可能出现链接问题 |
| UI | `UTKUserWidgetBase` 为空 | `Source/TKGamea/Ui/TKUserWidgetBase.h:13` | 没有身份、手牌、阶段、濒死、结算 UI |
| 配置 | `DefaultEngine.ini` 只有项目重定向；未配置默认 GameMode | `Config/DefaultEngine.ini:79` | 当前 C++ GameMode 可能未被地图/项目默认使用 |

## 2. 对照需求缺少的核心功能

### 2.1 P0：身份基础模式核心闭环缺口

| 需求项 | 当前缺口 | 建议落点 |
| --- | --- | --- |
| 模式配置 | 没有 `ModeType`、人数、抽牌数、是否启用武将等配置 | 新增 `FTKModeConfig` / `UTKModeDataAsset`，由 GameMode 在 `StartGame()` 加载 |
| 玩家状态 | 没有 `PlayerId`、座位、存活、身份、体力、手牌、装备区、判定区 | 新增 `ATKPlayerStateBase` 或 `UTKPlayerSanguoshaStateComponent`；公开状态放 PlayerState，私有手牌走 owner-only |
| 身份分配 | 没有身份枚举、身份牌人数表、随机分配、主公公开 | 新增 `ETKIdentity`、身份人数 DataTable、`UTKIdentityRuleComponent` |
| 开局流程 | 没有创建对局、加入玩家、洗牌、发初始牌、设置主公先手 | GameMode 负责权威初始化，GameState 复制结果 |
| 牌堆与牌区 | 没有游戏牌实例、牌堆、弃牌堆、手牌、装备区、判定区 | 新增 `FTKCardInstance`、`UTKCardZoneComponent`、`UTKDeckComponent` |
| 回合阶段 | 没有准备、判定、摸牌、出牌、弃牌、结束六阶段 | 扩展 `UTKTurnComponentBase` 为阶段状态机 |
| 摸牌/弃牌 | 没有摸 2、弃至体力值、死亡清区等牌移动 | 卡牌区组件提供 `DrawCards()`、`DiscardCards()`、`MoveCard()` |
| 出牌限制 | 没有每阶段 1 张【杀】、判定区同名限制 | 回合组件记录阶段统计；卡牌规则组件校验目标和限制 |
| 体力/濒死 | 没有体力上限、当前体力、伤害、濒死、求桃/酒响应 | 新增生命组件和响应窗口组件 |
| 死亡奖惩 | 没有亮身份、弃置区域、杀反贼摸 3、主公杀忠臣弃牌 | `ResolveDeath()` 后派发奖惩事件，再做胜负判定 |
| 胜负判定 | 没有主忠/反贼/内奸胜利逻辑和对局结束状态 | 身份规则组件提供 `EvaluateIdentityVictory()` |
| UI/调试可见性 | 没有阶段、身份、手牌、结算显示 | 先做调试面板或日志，再接正式 UMG |

### 2.2 P1：武将与技能框架缺口

| 需求项 | 当前缺口 | 建议落点 |
| --- | --- | --- |
| 武将配置 | 没有武将 ID、势力、性别、体力、技能、主公候选配置 | `UTKGeneralDataAsset` 或 DataTable |
| 选将流程 | 没有主公先选、其他玩家候选、确认后统一亮出 | 新增选将状态和 RPC，先支持身份模式 |
| 体力修正 | 没有 5 人及以上主公体力 +1 | 生命初始化由武将和身份规则共同计算 |
| 技能入口 | 没有技能触发点、注册和优先级 | 用 GameplayTag 定义事件点，技能对象监听强类型事件 |
| 禁用武将 | 没有无武将模式下 4 血无技能路径 | 模式配置决定是否进入选将 |

### 2.3 P2/P3：国战模式缺口

国战相关功能当前均未实现，包括双将选择、暗置/明置、势力判断、野心家、国战体力、国战死亡奖惩、国战胜负、阴阳鱼、先驱、珠联璧合、鏖战、君主和查看下家副将可选规则。建议在身份模式闭环稳定后再开发，否则会让隐藏信息、势力判断和响应窗口同时变复杂。

## 3. 当前代码中的即时风险

1. `UTKEventComponentBase::Get()` 使用 `check(EventComponent)`，但 `ATKPlayerControllerBase` 当前没有创建 `UTKEventComponentBase`。如果蓝图或外部没有手动挂组件，运行时会直接断言失败。
2. `UTKCardBase::ActiveCard()` 只有声明没有定义。只要有代码调用该函数，就可能产生链接错误。
3. `ATKGameStateBase::TurnComponent` 是 private 且只给 `UTKTurnComponentBase` friend。后续其他规则模块如果需要访问回合组件，会被迫绕路。建议提供 BlueprintCallable/BlueprintPure getter。
4. `FEventContext` 使用 `Params_i`、`Params_g`、`Params_O` 三组松散参数，早期方便，但规则增长后难以测试和维护。建议保留通用事件 Tag，同时为伤害、用牌、濒死、死亡等核心事件建立强类型 struct。
5. 当前没有 `PlayerState` 层。三国杀有大量“公开状态 + 私有信息”，只放在 PlayerController 或 GameState 都容易产生复制和隐藏信息问题。

## 4. 推荐架构

### 4.1 权威端与状态归属

| 层级 | 职责 |
| --- | --- |
| `ATKGameModeBase` | 只在服务器/权威端执行：创建对局、校验开始条件、分配身份、发牌、胜负结算 |
| `ATKGameStateBase` | 复制公开对局状态：模式、当前回合、当前阶段、全局标记、公开牌区、胜负结果 |
| `ATKPlayerStateBase` | 复制玩家公开状态：座位、存活、公开身份、体力、装备区、判定区、明置武将、公开标记 |
| `ATKPlayerControllerBase` | 处理本地输入和私有视角：自己的暗身份、手牌、候选武将、响应窗口 |
| `UTKTurnComponentBase` | 阶段状态机：当前玩家、当前阶段、阶段统计、进入/退出阶段事件 |
| `UTKCardZoneComponent` | 牌堆、弃牌堆、手牌、装备区、判定区的权威移动与复制过滤 |
| `UTKRuleEventComponent` | 事件派发与响应窗口：用牌、伤害、濒死、死亡、胜负、技能触发 |

隐藏信息建议遵循：服务器保存完整状态；客户端只收到自己有权知道的数据。手牌、暗身份、暗将、候选武将不要直接作为全局公开 replicated 数组。

### 4.2 建议新增核心类型

```cpp
UENUM(BlueprintType)
enum class ETKGameModeType : uint8
{
    Identity,
    NationalWar
};

UENUM(BlueprintType)
enum class ETKTurnPhase : uint8
{
    Prepare,
    Judge,
    Draw,
    Play,
    Discard,
    Finish
};

UENUM(BlueprintType)
enum class ETKIdentity : uint8
{
    Unknown,
    Lord,
    Loyalist,
    Rebel,
    Renegade
};

UENUM(BlueprintType)
enum class ETKCardZone : uint8
{
    Deck,
    DiscardPile,
    Hand,
    Equipment,
    Judgement,
    OutOfGame
};
```

同时建议增加：

| 类型 | 用途 |
| --- | --- |
| `FTKModeConfig` | 对局人数、模式、初始手牌、摸牌数、是否启用武将/技能 |
| `FTKCardDef` | 卡牌配置：名称、类型、花色、点数、时机、效果 Tag |
| `FTKCardInstance` | 单张牌实例：实例 ID、配置 ID、拥有者、所在区域 |
| `FTKPlayerPublicState` | 可公开复制的玩家状态 |
| `FTKDamageContext` | 伤害来源、目标、数值、原因、卡牌 |
| `FTKDyingContext` | 濒死角色、救援需求、可响应玩家、当前响应状态 |
| `FTKGameResult` | 胜利阵营、获胜玩家、结束原因 |

### 4.3 事件设计建议

继续使用 GameplayTag 是合理的，但不要只依赖松散参数数组。建议事件分两层：

1. 通用事件总线：`Event.Game.Started`、`Event.Phase.Begin`、`Event.Card.Used`、`Event.Player.Dying`、`Event.Player.Death`、`Event.Game.Ended`。
2. 强类型上下文：每类核心事件都有专用 struct，并在事件总线中作为 payload 或 UObject wrapper 传递。

这样后续技能可以按 Tag 注册，又能在 C++ 和自动化测试中安全读取字段。

### 4.4 回合状态机建议

`UTKTurnComponentBase` 应尽快补齐以下接口：

| 接口 | 说明 |
| --- | --- |
| `StartTurn(APlayerState*)` | 设置当前玩家并进入准备阶段 |
| `EnterPhase(ETKTurnPhase)` | 进入指定阶段，派发阶段开始事件 |
| `AdvancePhase()` | 按准备、判定、摸牌、出牌、弃牌、结束推进 |
| `EndTurn()` | 清理阶段统计，寻找下一名存活玩家 |
| `CanUseSlash(PlayerId)` | 检查本出牌阶段主动【杀】次数 |
| `ApplyDiscardLimit(PlayerId)` | 弃牌阶段校验手牌数不超过体力 |

阶段推进不要直接写死所有效果。默认行为可以由阶段组件调用规则服务完成，技能和装备通过事件插入修正。

## 5. 分阶段落地建议

### M1：身份基础模式最小闭环

优先实现可从开局走到胜负的最小规则，不接完整卡牌效果和武将技能。

1. 新增身份、阶段、牌区、游戏结果等枚举。
2. 新增 PlayerState，保存座位、存活、身份可见性、体力、公开牌区。
3. GameMode 增加 `StartIdentityGame()`：人数校验、身份分配、主公公开、发 4 张手牌、设置主公先手。
4. TurnComponent 实现 6 阶段推进，摸牌阶段默认摸 2。
5. CardZone 实现牌堆/弃牌堆/手牌移动，先用调试牌堆即可。
6. 实现基础死亡和身份胜负判定。
7. 用日志或调试 UI 显示当前玩家、阶段、身份可见性、手牌数、体力、胜负结果。

### M2：基础卡牌与濒死响应

1. 实现【杀】主动使用次数统计。
2. 实现【桃】【酒】濒死救援窗口。
3. 实现判定区同名延时锦囊限制。
4. 实现杀反贼摸 3、主公杀忠臣弃置全部区域牌。
5. 为 ID-001 至 ID-010 建立自动化测试或可复现调试命令。

### M3：武将框架

1. 增加武将 DataAsset/DataTable。
2. 增加选将候选、选择、确认、亮将流程。
3. 实现武将体力上限和 5 人及以上主公 +1。
4. 建立技能触发点，不急于实现完整武将技能。

### M4/M5：国战

身份模式稳定后再进入国战。国战优先做双将、暗置/明置、势力、国战体力、死亡奖惩和胜负；最后再做野心家、标记、鏖战和君主规则。

## 6. 建议的目录演进

当前目录较少，建议在不大改项目名的前提下逐步扩展：

```text
Source/TKGamea/
  Core/
    TKGameModeBase.*
    TKGameStateBase.*
    TKPlayerStateBase.*
    TKPlayerControllerBase.*
  Rules/
    TKIdentityRuleComponent.*
    TKNationalWarRuleComponent.*
    TKRuleEventComponent.*
    TKHealthComponent.*
    TKResponseWindowComponent.*
  Turn/
    TKTurnComponentBase.*
    TKTurnTypes.h
  Cards/
    TKCardTypes.h
    TKCardBase.*
    TKCardZoneComponent.*
    TKDeckComponent.*
  Generals/
    TKGeneralTypes.h
    TKGeneralDataAsset.*
    TKGeneralSelectionComponent.*
  UI/
    TKUserWidgetBase.*
```

如果短期不想移动已有文件，可以先在现有 `Core` 和 `Game` 下补功能，等 M1/M2 稳定后再拆目录。

## 7. 推荐测试清单

优先覆盖需求文档中的身份模式用例：

| 编号 | 测试目标 | 建议验证点 |
| --- | --- | --- |
| ID-001 | 开局身份分配 | 主公公开，其他身份仅本人可见 |
| ID-002 | 主公死亡且内奸唯一存活 | 内奸胜利 |
| ID-003 | 主公死亡且反贼存活 | 反贼胜利 |
| ID-004 | 反贼和内奸全部死亡 | 主忠胜利 |
| ID-005 | 杀死反贼 | 击杀者摸 3 张 |
| ID-006 | 主公杀死忠臣 | 主公弃置手牌、装备区、判定区 |
| ID-007 | 出牌阶段连续使用两张【杀】 | 第二张被拒绝 |
| ID-008 | 判定区已有同名牌 | 同名牌不能再次放入 |
| ID-009 | 弃牌阶段手牌超过体力 | 必须弃至当前体力 |
| ID-010 | 体力降至 0 后使用【桃】 | 回复到至少 1 并退出濒死 |

测试实现建议：

1. 规则层尽量写成 C++ 可调用服务，方便自动化测试绕过 UI。
2. 给 GameMode 提供 Debug Start 参数，允许固定身份、固定牌堆、固定击杀者。
3. 每个阶段和关键事件输出日志，便于复盘规则顺序。

## 8. 总体优先级

| 优先级 | 工作 | 原因 |
| --- | --- | --- |
| P0 | PlayerState、ModeConfig、身份分配、牌区、回合六阶段、基础胜负 | 没有这些无法形成任何可玩闭环 |
| P0 | 修复 EventComponent 未挂载与 CardBase 未定义风险 | 避免早期运行/链接问题 |
| P1 | 濒死响应、死亡奖惩、【杀】次数、弃牌限制 | 完成身份模式核心规则 |
| P1 | 调试 UI/日志与自动化测试 | 规则游戏非常依赖可复现验证 |
| P2 | 武将配置与技能触发框架 | 为后续扩展铺路 |
| P3 | 国战核心与扩展规则 | 复杂度高，应在基础身份局稳定后接入 |

## 9. 下一步建议

建议下一步直接实现 M1 的基础类型与状态承载：

1. 新增 `TKCardTypes.h`、`TKTurnTypes.h`、`TKPlayerStateBase.h/cpp`。
2. 给 `ATKGameStateBase` 增加公开 getter 和对局状态字段。
3. 给 `ATKPlayerControllerBase` 创建 `UTKEventComponentBase`，避免 `Get()` 断言风险。
4. 补上 `UTKCardBase::ActiveCard()` 的空实现或改成 `BlueprintNativeEvent`。
5. 开始实现 `UTKTurnComponentBase` 的阶段枚举、当前玩家、当前阶段和 `AdvancePhase()`。

完成这些后，工程才真正具备承接身份分配、发牌、摸牌、弃牌和胜负结算的基础。
