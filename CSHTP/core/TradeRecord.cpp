#include"TradeRecord.h"

// 本轮只落「契约」：字段、方法签名与语义写在 TradeRecord.h 里。
// 实现（Create 的校验与快照拷贝、IsParticipant 的判定）下一步与
// TradeRecordStore、Item 状态迁移方法一起写，避免先写实现、
// 再因模型调整而返工。
